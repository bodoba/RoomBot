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
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "serial.h"

#define WRITE_TIMEOUT_MS 200

static int fdSerial = -1;                       /**< Open device or -1 */
/* Serialize writes with operations that close or reconfigure the device. */
static pthread_mutex_t txLock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Convert a supported numeric baud rate to its termios speed constant.
 *
 * @param baud Numeric baud rate to convert.
 * @return The matching termios speed constant, or B0 if the baud rate is unsupported.
 */
static speed_t baudToSpeed(unsigned baud) {
	switch (baud) {
	case SERIAL_BAUD_19200:  return B19200;
	case SERIAL_BAUD_57600:  return B57600;
	case SERIAL_BAUD_115200: return B115200;
	default:                 return B0;
	}
}

/**
 * @brief Configure a file descriptor for raw Roomba serial communication.
 *
 * Sets the device to raw 8N1 mode with no flow control, enables the receiver,
 * applies the requested baud rate, and flushes pending input and output.
 *
 * @param fd Open file descriptor for a terminal device.
 * @param baud Supported baud rate (19200, 57600, or 115200).
 * @return 0 on success; -1 on failure with errno set.
 */
int serialConfigure(int fd, unsigned baud) {
	struct termios tio;
	speed_t speed = baudToSpeed(baud);

	if (speed == B0) {
		errno = EINVAL;
		return -1;
	}
	if (tcgetattr(fd, &tio) < 0)
		return -1;

	/* Configure a raw 8N1 link; poll() is responsible for read-side waiting. */
	cfmakeraw(&tio);                        /* no echo, no line editing */
	tio.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CRTSCTS);
	tio.c_cflag |= CS8 | CLOCAL | CREAD;    /* 8N1, ignore modem lines */
	tio.c_cc[VMIN] = 0;                     /* poll() handles waiting */
	tio.c_cc[VTIME] = 0;

	if (cfsetispeed(&tio, speed) < 0 || cfsetospeed(&tio, speed) < 0)
		return -1;
	if (tcsetattr(fd, TCSANOW, &tio) < 0)
		return -1;
	return tcflush(fd, TCIOFLUSH);
}

/**
 * @brief Open and configure the serial device used by this module.
 *
 * Opens the device for non-blocking read/write access and requests exclusive
 * access where supported. Only one device may be open at a time.
 *
 * @param dev Path to the serial device.
 * @param baud Supported baud rate (19200, 57600, or 115200).
 * @return The open file descriptor on success; -1 on failure with errno set.
 */
int serialOpen(const char *dev, unsigned baud) {
	int fd, err;

	if (fdSerial >= 0) {
		errno = EBUSY;
		return -1;
	}
	fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0)
		return -1;
	if (serialConfigure(fd, baud) < 0) {
		/* Preserve the configuration error while releasing the new descriptor. */
		err = errno;
		close(fd);
		errno = err;
		return -1;
	}
	/* Some drivers do not support TIOCEXCL, so exclusivity is best effort. */
	(void)ioctl(fd, TIOCEXCL);              /* exclusive access, best effort */
	fdSerial = fd;
	return fd;
}

/**
 * @brief Close the serial device if it is open.
 *
 * Waits for any write operation holding the transmit lock to finish before
 * closing the device.
 *
 * @return None.
 */
void serialClose(void) {
	/* Keep the descriptor valid until any in-progress write has finished. */
	pthread_mutex_lock(&txLock);
	if (fdSerial >= 0) {
		close(fdSerial);
		fdSerial = -1;
	}
	pthread_mutex_unlock(&txLock);
}

/**
 * @brief Get the file descriptor for the currently open serial device.
 *
 * @return The open file descriptor, or -1 if no device is open.
 */
int serialFd(void) {
	return fdSerial;
}

/**
 * @brief Change the baud rate of the currently open serial device.
 *
 * Drains pending output before applying the new serial configuration.
 *
 * @param baud Supported baud rate (19200, 57600, or 115200).
 * @return 0 on success; -1 on failure with errno set.
 */
int serialSetBaud(unsigned baud) {
	int rc;

	if (fdSerial < 0) {
		errno = EBADF;
		return -1;
	}
	pthread_mutex_lock(&txLock);
	/* Drain at the old rate so queued bytes are not sent with the new settings. */
	tcdrain(fdSerial);                      /* let pending bytes leave first */
	rc = serialConfigure(fdSerial, baud);
	pthread_mutex_unlock(&txLock);
	return rc;
}

/**
 * @brief Write all provided bytes to the serial device.
 *
 * Retries interrupted and temporarily blocked writes, waiting for output
 * readiness for up to WRITE_TIMEOUT_MS when the device cannot accept more data.
 *
 * @param buf Address of the bytes to transmit.
 * @param len Number of bytes to transmit.
 * @return The number of bytes written (len) on success; -1 on failure with
 *         errno set.
 */
ssize_t serialWrite(const uint8_t *buf, size_t len) {
	size_t done = 0;
	ssize_t n;
	struct pollfd pfd;

	if (fdSerial < 0) {
		errno = EBADF;
		return -1;
	}
    
	pthread_mutex_lock(&txLock);
	pfd.fd = fdSerial;
	pfd.events = POLLOUT;
	while (done < len) {
		/* A non-blocking write may accept only part of the buffer. */
		n = write(fdSerial, buf + done, len - done);
		if (n > 0) {
			done += (size_t)n;
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		if (n < 0 && errno == EAGAIN) {
			/* Wait for room in the device's output queue before retrying. */
			if (poll(&pfd, 1, WRITE_TIMEOUT_MS) > 0)
				continue;
			errno = ETIMEDOUT;
		}
		pthread_mutex_unlock(&txLock);
		return -1;
	}
	pthread_mutex_unlock(&txLock);
	return (ssize_t)done;
}

/**
 * @brief Discard unread input from the serial device.
 *
 * Does nothing if the serial device is not open.
 *
 * @return None.
 */
void serialFlushInput(void) {
	if (fdSerial >= 0)
		tcflush(fdSerial, TCIFLUSH);
}