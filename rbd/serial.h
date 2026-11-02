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
#ifndef SERIAL_H
#define SERIAL_H

#include <stddef.h>  
#include <stdint.h>
#include <sys/types.h>

/** Baud rates supported by the Roomba Open Interface. */
#define SERIAL_BAUD_19200   19200
#define SERIAL_BAUD_57600   57600
#define SERIAL_BAUD_115200  115200

/* *********************************************************************************** *
 * @brief Open and configure the serial device.
 *
 * The device is opened read/write, non-blocking, without becoming the
 * controlling terminal, and with exclusive access.
 *
 * @param dev  Device path, e.g. "/dev/ttyS2".
 * @param baud Baud rate (19200, 57600 or 115200).
 * @return The file descriptor (>= 0) on success, -1 on error (errno set).
 * *********************************************************************************** */
int serialOpen(const char *dev, unsigned baud);

/* *********************************************************************************** *
 * @brief Close the serial device.
 * *********************************************************************************** */
void serialClose(void);

/* *********************************************************************************** *
 * @brief Get the file descriptor of the open serial device.
 *
 * Use it with poll() and read(). Do not write to it directly, use
 * serialWrite() instead.
 *
 * @return File descriptor, or -1 if the device is not open.
 * *********************************************************************************** */
int serialFd(void);

/* *********************************************************************************** *
 * @brief Set the TTY parameters for talking to the Roomba.
 *
 * Raw mode, 8N1, no flow control, no echo, receiver enabled, modem lines
 * ignored, VMIN=0/VTIME=0. Pending input and output are discarded.
 *
 * @param fd   Open file descriptor of a tty.
 * @param baud Baud rate (19200, 57600 or 115200).
 * @return 0 on success, -1 on error (errno set, EINVAL for a bad baud rate).
 * *********************************************************************************** */
int serialConfigure(int fd, unsigned baud);

/* *********************************************************************************** *
 * @brief Change the baud rate of the open device.
 *
 * Call this after the Roomba was told to change its rate (OI opcode 129)
 * and after it had time to switch (at least 100 ms).
 *
 * @param baud New baud rate.
 * @return 0 on success, -1 on error.
 * *********************************************************************************** */
int serialSetBaud(unsigned baud);

/* *********************************************************************************** *
 * @brief Write bytes to the Roomba (thread safe).
 *
 * Blocks until all bytes are written or a 200 ms timeout occurs.
 *
 * @param buf Data to send.
 * @param len Number of bytes.
 * @return Number of bytes written (== len) on success, -1 on error.
 * *********************************************************************************** */
ssize_t serialWrite(const uint8_t *buf, size_t len);

/* *********************************************************************************** *
 * @brief Discard all unread input.
 *
 * Useful before starting a stream or after a mode change.
 * *********************************************************************************** */
void serialFlushInput(void);

#endif