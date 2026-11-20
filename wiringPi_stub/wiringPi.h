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
#ifndef WIRINGPI_STUB_H
#define WIRINGPI_STUB_H

#define INPUT 0
#define OUTPUT 1
#define LOW 0
#define HIGH 1

/**
 * @brief Stub for wiringPiSetup().
 *
 * This implementation performs no hardware setup and is for compile tests only.
 *
 * @return Always 0.
 */
int wiringPiSetup(void);

/**
 * @brief Stub for pinMode().
 *
 * This implementation does not access GPIO hardware.
 *
 * @param pin GPIO pin number.
 * @param mode Pin mode.
 */
void pinMode(int pin, int mode);

/**
 * @brief Stub for digitalWrite().
 *
 * This implementation does not access GPIO hardware.
 *
 * @param pin GPIO pin number.
 * @param value Pin level.
 */
void digitalWrite(int pin, int value);

#endif /* WIRINGPI_STUB_H */
