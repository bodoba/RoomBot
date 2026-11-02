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
#ifndef MQTT_H
#define MQTT_H

#include <stdarg.h>
#include <ctype.h>
#include <stdbool.h>

/* *********************************************************************************** *
 * Useful constants                                  
 * *********************************************************************************** */
#define MAX_MQTT_MESSAGE 1024  /**< Maximum lenght of a MQTT message */

/* *********************************************************************************** *
 * @struct mqttIncoming_t
 * @brief Struct to register handler for MQTT subscriptions. mqttInit() will register
 *        the sbscribtions.
 * 
 * Callback prototype:   void mqttCB(char *payload,      MQTT payload  
 *                                   int   payloadlen,   Lenght of payload
 *                                   char  *topic,       Subcription topic
 *                                   void *user_data     User defined argument
 *                            );
 * *********************************************************************************** */
 typedef struct mqttIncoming_t {
    const char* topic;                          /**< MQTT topic to subscribe to        */
    void  (*handler)(char*, int, char*, void*); /**< callback function                 */
    void  *user_data;                           /**< user defined argument             */
} mqttIncoming_t;

/* *********************************************************************************** *
 * @brief Setup connection to MQTT broker
 * 
 * Initilizes mosquitto library, connects to MQTT broker and subscribes to topics 
 * given in subscritions
 * 
 * @param[in] broker        IP Address of MQTT broker
 * @param[in] port          Port number of MQTT broker
 * @param[in] keepalive     Keepalive counter of MQTT broker
 * @param[in] subscriptions Array of MQTT subscriptions
 * 
 * @return true on success, false on error
 * *********************************************************************************** */
bool mqttInit(const char* broker, int port, int keepalive, mqttIncoming_t *subscriptions);

/* *********************************************************************************** *
 * @brief Terminate connection to MQTT broker                                   
 * *********************************************************************************** */
void mqttEnd(void );

/* *********************************************************************************** *
 * @brief Publish a formatted message with given topic
 * 
 * @param topic   Topic of MQTT message
 * @param format  A printf-style format string.
 * @param ...     Values referenced by the format string.
 * 
 * @return true on success, false on error
 * *********************************************************************************** */
bool mqttPublish(const char *topic, const char *format,...);

/* *********************************************************************************** *
 * @brief Publish a formatted message with given topic
 * 
 * @param topic   Topic of MQTT message
 * @param payload Message payload
 * @param len     Payload length
 * @param qos     0, 1 or 2 indicating the Quality of Service 
 * @param retain  Set to true to make the message retained
 * 
 * @return true on success, false on error
 * *********************************************************************************** */
bool mqttPublishRaw(const char* topic, const char *payload, int len, int qos, bool retain);

#endif