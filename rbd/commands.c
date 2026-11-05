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
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>

#include "commands.h"
#include "serial.h"

/**
 * @brief Send a complete command sequence and wait until it has been transmitted.
 *
 * The whole sequence is written with one serialWrite() call, so it cannot be
 * interleaved with bytes from other threads.
 *
 * @param seq Bytes to send.
 * @param len Number of bytes.
 *
 * @return 0 on success, -1 on error (errno set).
 */
static int sendSequence(const uint8_t *seq, size_t len) {
	int fd = serialFd();

	if (fd < 0) {
		errno = EBADF;
		return -1;
	}
	if (serialWrite(seq, len) != (ssize_t)len)
		return -1;
	return tcdrain(fd);             /* wait until the UART has sent everything */
}

/* *********************************************************************************** *
 * P U B L I C  I N T E R F A C E
 * *********************************************************************************** */

int commandPowerDown(void) {
	static const uint8_t seq[] = {
		OI_OP_DRIVE_DIRECT, 0, 0, 0, 0,     /* right = 0 mm/s, left = 0 mm/s */
		OI_OP_MOTORS, 0,                    /* brushes and vacuum off        */
		OI_OP_STREAM_PAUSE, 0,              /* stop the sensor stream        */
		OI_OP_POWER                         /* power down                    */
	};

	return sendSequence(seq, sizeof(seq));
}

int commandStart(void) {
	static const uint8_t start[] = {OI_OP_START};
	static const uint8_t safe[]  = {OI_OP_SAFE};

	serialFlushInput();
	if (sendSequence(start, sizeof(start)) < 0)
		return -1;
	usleep(OI_MODE_CHANGE_US);
	if (sendSequence(safe, sizeof(safe)) < 0)
		return -1;
	usleep(OI_MODE_CHANGE_US);
	return 0;
}

int commandDisplay(const char *text) {
	uint8_t seq[1 + DISPLAY_DIGITS];
	unsigned char c;
	size_t i;

	seq[0] = OI_OP_DIGIT_LEDS_ASCII;
	for (i = 0; i < DISPLAY_DIGITS; i++) {
		c = ' ';
		if (text != NULL && *text != '\0') {
			c = (unsigned char)*text++;
			if (c < 32 || c > 126)
				c = ' ';        /* not printable: show blank */
		}
		seq[1 + i] = c;
	}
	return sendSequence(seq, sizeof(seq));
}

int commandDisplayRaw(const uint8_t digits[DISPLAY_DIGITS]) {
	uint8_t seq[1 + DISPLAY_DIGITS];
	size_t i;

	if (digits == NULL) {
		errno = EINVAL;
		return -1;
	}
	seq[0] = OI_OP_DIGIT_LEDS_RAW;
	for (i = 0; i < DISPLAY_DIGITS; i++)
		seq[1 + i] = digits[i] & 0x7f;  /* bit 7 is unused */
	return sendSequence(seq, sizeof(seq));
}

int commandGetMode(OiMode *mode) {
	static const uint8_t query[] = {OI_OP_SENSORS, OI_PKT_OI_MODE};
	struct pollfd pfd;
	uint8_t reply;
	ssize_t n;
	int rc;
	int fd = serialFd();

	if (mode == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (fd < 0) {
		errno = EBADF;
		return -1;
	}
	serialFlushInput();                     /* drop stale bytes */
	if (sendSequence(query, sizeof(query)) < 0)
		return -1;

	pfd.fd = fd;
	pfd.events = POLLIN;
	do {
		rc = poll(&pfd, 1, MODE_REPLY_TIMEOUT_MS);
	} while (rc < 0 && errno == EINTR);
	if (rc == 0) {
		errno = ETIMEDOUT;              /* Roomba asleep or OI not started */
		return -1;
	}
	if (rc < 0)
		return -1;

	n = read(fd, &reply, 1);
	if (n != 1) {
		if (n >= 0)
			errno = EIO;
		return -1;
	}
	if (reply > OI_MODE_FULL) {
		errno = EPROTO;
		return -1;
	}
	*mode = (OiMode)reply;
	return 0;
}

const char *commandModeName(OiMode mode) {
	switch (mode) {
	case OI_MODE_OFF:     return "OFF";
	case OI_MODE_PASSIVE: return "PASSIVE";
	case OI_MODE_SAFE:    return "SAFE";
	case OI_MODE_FULL:    return "FULL";
	default:              return "UNKNOWN";
	}
}