/* *********************************************************************************** */
/*                                                                                     */
/*  Copyright (c) 2026 by Bodo Bauer <bb@bb-zone.com>                                  */
/*                                                                                     */
/*  This program is free software: you can redistribute it and/or modify               */
/*  it under the terms of the GNU General Public License as published by               */
/*  the Free Software Foundation, either version 3 of the License, or                  */
/*  (at your option) any later version.                                                */
/*                                                                                     */
/*  This program is distributed in the hope that it will be useful,                    */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of                     */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                      */
/*  GNU General Public License for more details.                                       */
/*                                                                                     */
/*  You should have received a copy of the GNU General Public License                  */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.              */
/* *********************************************************************************** */

#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <syslog.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include "wiringPi.h"

#include "rbd.h"
#include "logging.h"
#include "daemon.h"
#include "mqtt.h"
#include "sensors.h"
#include "serial.h"
#include "commands.h"
#include "signals.h"
#include "remotecontrol.h"

/**
 * Some globals we can't do without... ;)
 */
int    debug              = DEBUG;             // debug level (exported)
bool   foreground         = false;             // run in foreground, not as daemon

/**
 * MQTT subscriptions
 */
mqttIncoming_t mqttSubscriptions[] = {
    {"roombot/cmd/drive",        &mqttDrive,       NULL},
    {"roombot/cmd/start",        &mqttStart,       NULL},
    {"roombot/cmd/power_down",    &mqttPowerDown,  NULL},
    {"roombot/cmd/display",      &mqttDisplay,     NULL},
    {NULL, NULL, NULL},
};

/**
 * @brief Cleanup called by the signal thread after the Roomba power-down.
 */
static void shutdownHook(void) {
    commandDisplay(NULL);
    mqttEnd();
    shutdown_daemon();
}

/* *********************************************************************************** *
 *                                  M  A  I  N
 * *********************************************************************************** */ 
int main( int argc, char *argv[] ) {
    
    /* ******************************************************************************* *
     * Process command line arguments
     * ******************************************************************************* */ 
    for (int i=0; i<argc; i++) {
        if (!strcmp(argv[i], "-d")) {          // '-d' turns debug mode on
            debug++;
        }
        
        if (!strcmp(argv[i], "-f")) {          // '-f' forces forground mode
            foreground=true;
        }
    }
    
    /* ******************************************************************************* *
     * Initialize logging
     * ******************************************************************************* */ 
    initLog(!foreground);
    setLogLevel(LOG_NOTICE+debug);
        
    /* ******************************************************************************* *
     * Initialize signal handling
     * ******************************************************************************* */ 
    if (signalsInit() != 0) {
        writeLog(LOG_ERR, "Cannot initialize signal handling: %s", strerror(errno));
        exit(1);
    }

    /* ******************************************************************************* *
     * Daemonize if not in foreground mode
     * ******************************************************************************* */ 
    if (!foreground) {
        // run in background
        daemonize(PID_FILE);
    } else {
        writeLog(LOG_NOTICE, "Running in foreground");
    }

    /* ******************************************************************************* *
     * Start signal thread
     * ******************************************************************************* */ 
    if (signalsStart(shutdownHook) != 0) {
        writeLog(LOG_ERR, "Cannot start signal thread: %s", strerror(errno));
        exit(1);
    }
    
    /* ******************************************************************************* *
     * Start Watchdog thread
     * ******************************************************************************* */ 
    watchdogStart();    
    
    /* ******************************************************************************* *
     * Initialize WiringPi (before MQTT, the start handler uses the GPIO)
     * ******************************************************************************* */ 
    wiringPiSetup();
    pinMode(DD_PIN, OUTPUT);
    digitalWrite(DD_PIN, HIGH);             /* idle level, no accidental pulse */

    /* ******************************************************************************* *
     * Initialize MQTT
     * ******************************************************************************* */ 
    if ( mqttInit( MQTT_BROKER, MQTT_PORT, MQTT_KEEPALIVE, mqttSubscriptions) ) {
        writeLog(LOG_INFO, "Connected to MQTT Broker %s:%04d", MQTT_BROKER, MQTT_PORT);
        mqttPublish("roombot/info/message", "Connected");
    } else {
        writeLog(LOG_ERR,  "Failed to connect to MQTT Broker %s:%04d", MQTT_BROKER, MQTT_PORT);
        exit(1);
    }
    
    /* ******************************************************************************* *
     * Turn on Roomba
     * ******************************************************************************* */ 
    digitalWrite(DD_PIN, LOW);                                      /*  turn on Roomba */
    sleep(1);
    digitalWrite(DD_PIN, HIGH);
    
    /* ******************************************************************************* *
     * Initialize serial interface, wake up and start the Roomba
     * ******************************************************************************* */  
    if (serialOpen(DEV_ROOMBA, SERIAL_BAUD_115200) < 0) {
        writeLog(LOG_ERR, "Cannot open %s: %s", DEV_ROOMBA, strerror(errno));
        exit(1);
    }
    
    if (commandWake() < 0 || commandStart() < 0) {
        writeLog(LOG_ERR, "Cannot start Roomba: %s", strerror(errno));
        exit(1);
    }
    
    commandDisplay("RUN");

    OiMode mode = -1;                       /* query BEFORE the stream starts */
    
    if (commandGetMode(&mode) == 0)
        writeLog(LOG_INFO, "Roomba started in mode %s (%d)", commandModeName(mode), mode);
    else
        writeLog(LOG_WARNING, "Mode query failed: %s", strerror(errno));

    commandStreamStart();

    /* ******************************************************************************* *
     * Start Sensor Thread
     * ******************************************************************************* */ 
    pthread_t threadSensorID;
    pthread_create(&threadSensorID, NULL, threadSensors, NULL);

    /* ******************************************************************************* *
     * From her to eternity... ;)
     * ******************************************************************************* */ 
    uint32_t heartbeat = 0; 
    for (;;) {
        sleep(1);
        mqttPublish("roombot/info/heartbeat", "0x%010lx", heartbeat);
        heartbeat++;
    }
    return 0;
}