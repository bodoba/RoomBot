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
#ifndef SENSORS_H
#define SENSORS_H

#include <pthread.h>

#define OI_STREAM_HEADER   19
#define OI_OP_STREAM       148
#define PUBLISH_PERIOD_MS  200                      /* one reading every 200 ms -> 5Hz */
#define JSON_MAX           384

/**
 * @brief Sensor Thread
 *
 * Read Roomba sensors and send the readings out via MQTT
 */
void *threadSensors(void *);

#endif