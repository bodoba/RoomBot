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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "remotecontrol.h"
#include "commands.h"
#include "mqtt.h"
#include "logging.h"
#include "json.h"
#include "watchdog.h"

#define PAYLOAD_MAX 128                 /**< Largest payload we accept */

/**
 * @brief Copy an MQTT payload into a NUL terminated buffer.
 *
 * MQTT payloads are not guaranteed to be NUL terminated.
 *
 * @param payload    MQTT payload.
 * @param payloadlen Length of the payload.
 * @param buf        Output buffer of PAYLOAD_MAX bytes.
 *
 * @return 0 on success, -1 if the payload is missing or too long.
 */
static int copyPayload(const char *payload, int payloadlen, char *buf) {
    if (payload == NULL || payloadlen <= 0 || payloadlen >= PAYLOAD_MAX)
        return -1;
    memcpy(buf, payload, (size_t)payloadlen);
    buf[payloadlen] = '\0';
    return 0;
}

void  mqttDrive(char *payload, int payloadlen, char *topic, void *user_data) {
    char buf[PAYLOAD_MAX];
    long velocity, radius;

    (void)topic;
    (void)user_data;
    writeLog(LOG_DEBUG, "mqttDrive() called with payload: %.*s", payloadlen, payload);

    if (copyPayload(payload, payloadlen, buf) < 0) {
        writeLog(LOG_WARNING, "mqttDrive(): invalid payload length %d", payloadlen);
        return;
    }
    /* The GUI sends "velocity", the header documents "speed": accept both. */
    if (jsonInt(buf, "velocity", &velocity) < 0 &&
        jsonInt(buf, "speed", &velocity) < 0) {
        writeLog(LOG_WARNING, "mqttDrive(): no velocity in payload");
        return;
    }
    if (jsonInt(buf, "radius", &radius) < 0)
        radius = DRIVE_RADIUS_STRAIGHT;     /* default: straight */

    /* Feed or disarm BEFORE sending, so a late watchdog stop cannot
     * override this command. */
    if (velocity != 0)
        watchdogFeed();
    else
        watchdogDisarm();

    if (commandDrive((int)velocity, (int)radius) < 0)
        writeLog(LOG_ERR, "mqttDrive(): commandDrive failed: %s", strerror(errno));
}

void  mqttStart(char *payload, int payloadlen, char *topic, void *user_data) {
    (void)topic;
    (void)user_data;
    writeLog(LOG_DEBUG, "mqttStart() called with payload: %.*s", payloadlen, payload);

    if (commandWake() < 0) {
        writeLog(LOG_ERR, "mqttStart(): commandWake failed: %s", strerror(errno));
        return;
    }
    if (commandStart() < 0) {
        writeLog(LOG_ERR, "mqttStart(): commandStart failed: %s", strerror(errno));
        return;
    }
    if (commandStreamStart() < 0)
        writeLog(LOG_ERR, "mqttStart(): commandStreamStart failed: %s", strerror(errno));
}

void  mqttPowerDown(char *payload, int payloadlen, char *topic, void *user_data) {
    (void)topic;
    (void)user_data;
    writeLog(LOG_DEBUG, "mqttPowerDown() called with payload: %.*s", payloadlen, payload);

    watchdogDisarm();

    if (commandPowerDown() < 0)
        writeLog(LOG_ERR, "mqttPowerDown(): commandPowerDown failed: %s", strerror(errno));
}

void  mqttDisplay(char *payload, int payloadlen, char *topic, void *user_data) {
    char buf[PAYLOAD_MAX];
    char text[DISPLAY_DIGITS + 1];

    (void)topic;
    (void)user_data;
    writeLog(LOG_DEBUG, "mqttDisplay() called with payload: %.*s", payloadlen, payload);

    if (copyPayload(payload, payloadlen, buf) < 0 ||
        jsonString(buf, "text", text, sizeof(text)) < 0) {
        writeLog(LOG_WARNING, "mqttDisplay(): no text in payload");
        return;
    }
    if (commandDisplay(text) < 0)
        writeLog(LOG_ERR, "mqttDisplay(): commandDisplay failed: %s", strerror(errno));
}