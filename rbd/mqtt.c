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
#include <mosquitto.h>
#include <stdio.h>
#include <string.h>

#include "mqtt.h"

/* *********************************************************************************** *
 * Module global variables
 * *********************************************************************************** */
static struct mosquitto *mosq = NULL;             /**< Handle to MQTT broker           */
static mqttIncoming_t *subscriptionList = NULL;   /**< List of topics to subscribe to  */

/* *********************************************************************************** *
 * @brief Dispatch incomming MQTT message
 * 
 * Select and call callback function that matches the topic
 * 
 * @param mos
 * @param userData
 * @param message
 * ************************************************************************************ */
void dispatchMessage(struct mosquitto *mos, void *userData, const struct mosquitto_message *message) {
    // identify callback functiion by matching topic
    int idx = 0;
    while (subscriptionList[idx].topic) {
        if(!strcmp(message->topic, subscriptionList[idx].topic)) {
            (subscriptionList[idx].handler)(message->payload,
                                            message->payloadlen,
                                            message->topic,
                                            subscriptionList[idx].user_data);
            break;
        }
        idx++;
    }
}

/* *********************************************************************************** *
 * P U B L I C  I N T E R F A C E
 * *********************************************************************************** */

bool mqttPublish ( const char *topic, const char *format, ...) {
    bool success = true;
    int  err;
    char message[MAX_MQTT_MESSAGE];
    va_list valist;

    va_start(valist, format);
    vsnprintf(message, (unsigned int)MAX_MQTT_MESSAGE, format, valist);
    va_end(valist);

   if ( mosq ) {
        err = mosquitto_publish( mosq, NULL, topic, strlen(message), message, 0, false);
        if ( err != MOSQ_ERR_SUCCESS) {
            success = false;
        }
    } else {
        success = false;
    }
    return success;
}

bool mqttPublishRaw(const char *topic, const char *payload, int len, int qos, bool retain)
{
    bool success = true;
    int  err;

   if ( mosq ) {
        err = mosquitto_publish( mosq, NULL, topic, len, payload, qos, retain);
        if ( err != MOSQ_ERR_SUCCESS) {
            success = false;
        }
    } else {
        success = false;
    }
    return success;
}

bool mqttInit( const char* broker, int port, int keepalive, mqttIncoming_t *subscriptions) {
    bool success = false;

    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if(mosq){
        if( mosquitto_connect(mosq, broker, port, keepalive) == MOSQ_ERR_SUCCESS ) {
            if( mosquitto_loop_start(mosq) == MOSQ_ERR_SUCCESS ) {
                mosquitto_message_callback_set(mosq, &dispatchMessage);
                subscriptionList = subscriptions;
                int idx = 0;
                while (subscriptionList[idx].topic) {
                    // writeLog(LOG_INFO, "Supscribe to MQTT topic: %s", subscriptionList[idx].topic);
                    mosquitto_subscribe( mosq, NULL, subscriptionList[idx].topic, 0);
                    idx++;
                }
                success = true;
            }
        }
    }
    return success;
}

void mqttEnd( void ) {
    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    mosq = NULL;
}