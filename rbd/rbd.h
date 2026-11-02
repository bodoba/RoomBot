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
#ifndef RBD_H
#define RBD_H

/* ----------------------------------------------------------------------------------- *
 * Default values for configurable parameter
 * ----------------------------------------------------------------------------------- */
#define DEBUG           0                        /* no debug info by default           */

/* ----------------------------------------------------------------------------------- *
 * System settings
 * ----------------------------------------------------------------------------------- */
#define DEV_ROOMBA      "/dev/ttyS2"
#define DD_PIN          6
#define PID_FILE        "/var/run/bt2usbd.pid"
#define MAX_PATH_LENGTH 256

#define MQTT_BROKER     "192.168.100.26"
#define MQTT_PORT       1883
#define MQTT_KEEPALIVE  60

/* ----------------------------------------------------------------------------------- *
 * export some globals
 * ----------------------------------------------------------------------------------- */
extern int debug;

#endif