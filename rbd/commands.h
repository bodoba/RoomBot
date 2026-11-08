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
#ifndef COMMANDS_H
#define COMMANDS_H
#include <stdint.h>

/** Roomba Open Interface opcodes used by this module.                             */
#define OI_OP_START             128     /**< Start the OI (Passive mode)           */
#define OI_OP_SAFE              131     /**< Enter Safe mode                       */
#define OI_OP_POWER             133     /**< Power down the Roomba                 */
#define OI_OP_DRIVE             137     /**< Drive with velocity and radius        */
#define OI_OP_MOTORS            138     /**< Switch brushes and vacuum             */
#define OI_OP_SENSORS           142     /**< Query a single sensor packet          */
#define OI_OP_DRIVE_DIRECT      145     /**< Drive wheels with individual speed    */
#define OI_OP_STREAM            148     /**< Start a sensor stream                 */
#define OI_OP_STREAM_PAUSE      150     /**< Pause (0) or resume (1) stream        */
#define OI_OP_DIGIT_LEDS_RAW    163     /**< Display: raw segment bits             */
#define OI_OP_DIGIT_LEDS_ASCII  164     /**< Display: ASCII characters             */

#define OI_PKT_OI_MODE           35     /**< Packet 35: current OI mode            */
#define DISPLAY_DIGITS            4     /**< Number of digits on the display       */
#define OI_MODE_CHANGE_US     20000     /**< OI needs >= 20 ms after a mode change */
#define MODE_REPLY_TIMEOUT_MS   100     /**< Max. wait for the mode reply          */

#define DRIVE_MAX_SPEED_MMS     300     /**< Speed limit enforced on the Pi        */
#define DRIVE_MAX_RADIUS_MM    2000     /**< Largest radius accepted               */
#define DRIVE_RADIUS_STRAIGHT 32768     /**< Special radius: drive straight        */

#define WAKE_PULSE_MS          1000     /**< BRC low time for the wake-up pulse    */
#define WAKE_BOOT_MS           2000     /**< Wait until the Roomba accepts OI      */

/** Open Interface modes as reported by packet 35. */
typedef enum {
	OI_MODE_OFF     = 0,
	OI_MODE_PASSIVE = 1,
	OI_MODE_SAFE    = 2,
	OI_MODE_FULL    = 3
} OiMode;

/**
 * @brief Start the Open Interface and switch the Roomba to Safe mode.
 *
 * Sends START (Passive mode), waits for the mode change, then sends SAFE so
 * that the wheels and motors can be controlled. Pending input is discarded
 * first.
 *
 * @note The Roomba must be awake. After commandPowerDown() the UART is dead
 *       until the wake-up pulse on the BRC pin has been sent (GPIO, not part
 *       of this module). While the Roomba is on the charger it may stay in
 *       Passive mode.
 *
 * @return 0 on success, -1 on error (errno set).
 */
int commandStart(void);

/**
 * @brief Power down the Roomba.
 *
 * Stops the wheels and the motors, pauses the sensor stream and sends the
 * Power command. The function waits until all bytes have left the UART.
 * The Roomba changes to Passive mode and powers down. To use it again, wake
 * it up (e.g. the wake-up pulse on the BRC pin) and send START.
 *
 * @note Needs an open serial device (see serialOpen()).
 *
 * @return 0 on success, -1 on error (errno set).
 */
int commandPowerDown(void);

/**
 * @brief Write text to the 4-digit display.
 *
 * The text is left aligned. Shorter strings are padded with spaces, longer
 * strings are truncated to 4 characters. Characters outside the range 32..126
 * are replaced by a space. Not every ASCII character can be shown on a
 * 7-segment display. The Roomba shows a substitute for those it cannot draw.
 *
 * @note Requires Safe or Full mode (see commandStart()).
 *
 * @param text Zero terminated string, NULL clears the display.
 *
 * @return 0 on success, -1 on error (errno set).
 */
int commandDisplay(const char *text);

/**
 * @brief Write raw segment bits to the 4-digit display.
 *
 * Bit 0..6 of each byte switch the segments A..G, bit 7 is ignored.
 *
 * @note Requires Safe or Full mode (see commandStart()).
 *
 * @param digits Four bytes, digits[0] is the leftmost digit.
 *
 * @return 0 on success, -1 on error (errno set, EINVAL if digits is NULL).
 */
int commandDisplayRaw(const uint8_t digits[DISPLAY_DIGITS]);

/**
 * @brief Query the current OI mode of the Roomba.
 *
 * Sends the Sensors command for packet 35 and waits up to 100 ms for the
 * one byte reply.
 *
 * @note Reads from the serial fd. Call it only while the sensor stream is not
 *       running or paused, and when no other thread reads the fd.
 *
 * @param mode Receives the mode on success.
 *
 * @return 0 on success, -1 on error (errno set: EINVAL for a NULL pointer,
 *         ETIMEDOUT if the Roomba does not answer, EPROTO for an invalid
 *         reply).
 */
int commandGetMode(OiMode *mode);

/**
 * @brief Convert an OI mode into a printable name.
 *
 * @param mode OI mode.
 *
 * @return "OFF", "PASSIVE", "SAFE", "FULL" or "UNKNOWN".
 */
const char *commandModeName(OiMode mode);

/**
 * @brief Drive the Roomba with a velocity and a turning radius.
 *
 * The velocity is limited to +/- DRIVE_MAX_SPEED_MMS. Special radii:
 * 32768 (or 32767) = straight, 1 = turn on the spot counter-clockwise (left),
 * -1 = turn on the spot clockwise (right). All other radii are limited to
 * +/- DRIVE_MAX_RADIUS_MM.
 *
 * @note Requires Safe or Full mode (see commandStart()).
 *
 * @param velocity Velocity in mm/s, negative = backwards.
 * @param radius   Turning radius in mm, see above.
 *
 * @return 0 on success, -1 on error (errno set).
 */
int commandDrive(int velocity, int radius);

/**
 * @brief Wake the Roomba up with a pulse on the BRC (Device Detect) pin.
 *
 * Pulses DD_PIN low for WAKE_PULSE_MS, waits WAKE_BOOT_MS for the Roomba to
 * boot and discards stale serial input. The Roomba is awake afterwards but
 * the OI is not started yet, so call commandStart() next.
 * Harmless if the Roomba is already awake.
 *
 * @note Blocks for about 3 s. Needs wiringPiSetup() and pinMode(DD_PIN, OUTPUT)
 *       to be done before.
 *
 * @return 0 on success, -1 on error (errno set, EBADF if the serial port is closed).
 */
int commandWake(void);

/**
 * @brief Start the sensor stream with the packets used by threadSensors().
 *
 * Packets 7-12, 22-26 and 35, one frame every 15 ms.
 *
 * @note Call it after commandStart(). Do not call commandGetMode() while
 *       the stream is running.
 *
 * @return 0 on success, -1 on error (errno set).
 */
int commandStreamStart(void);

#endif /* COMMANDS_H */