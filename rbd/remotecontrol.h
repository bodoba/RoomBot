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
#ifndef REMOTECONTROL_H
#define REMOTECONTROL_H

/**
 * @brief Drive the Roomba
 *
 * Drives the Roomba with the given speed and radius. The speed is in mm/s, the radius in mm.
 * Expects a JSON payload like {"velocity": 100, "radius": 200}.
 *
 * @param payload    MQTT payload
 * @param payloadlen Length of payload
 * @param topic      MQTT topic
 * @param user_data  User defined argument
 */
void  mqttDrive(char *payload, int payloadlen, char *topic, void *user_data); 


/**
 * @brief Start Roomba
 *
 * Turn on the Roomba and start the Open Interface. Expects an empty payload.
 * 
 * @param payload    MQTT payload
 * @param payloadlen Length of payload
 * @param topic      MQTT topic
 * @param user_data  User defined argument
 */
void  mqttStart(char *payload, int payloadlen, char *topic, void *user_data); 

/**
 * @brief Power down Roomba
 *
 * Turn off the Roomba. Expects an empty payload.
 * 
 * @param payload    MQTT payload
 * @param payloadlen Length of payload
 * @param topic      MQTT topic
 * @param user_data  User defined argument
 */
void  mqttPowerDown(char *payload, int payloadlen, char *topic, void *user_data);

/**
 * @brief Show given text on the Roomba display
 *
 * Expects a JSON payload like {"text": "ABCD"}. Display is limited to 4 characters
 * 
 * @param payload    MQTT payload
 * @param payloadlen Length of payload
 * @param topic      MQTT topic
 * @param user_data  User defined argument
 */
void  mqttDisplay(char *payload, int payloadlen, char *topic, void *user_data); 

#endif