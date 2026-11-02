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
static pthread_mutex_t txLock = PTHREAD_MUTEX_INITIALIZER;

/* *********************************************************************************** *
 * @brief Map a numeric baud rate to a termios speed constant.
 *
 * @param baud Baud rate.
 * @return Matching speed_t, or B0 if the rate is not supported.
 * *********************************************************************************** */
static speed_t baudToSpeed(unsigned baud) {
	switch (baud) {
	case SERIAL_BAUD_19200:  return B19200;
	case SERIAL_BAUD_57600:  return B57600;
	case SERIAL_BAUD_115200: return B115200;
	default:                 return B0;
	}
}

int serialConfigure(int fd, unsigned baud) {
	struct termios tio;
	speed_t speed = baudToSpeed(baud);

	if (speed == B0) {
		errno = EINVAL;
		return -1;
	}
	if (tcgetattr(fd, &tio) < 0)
		return -1;

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
		err = errno;
		close(fd);
		errno = err;
		return -1;
	}
	(void)ioctl(fd, TIOCEXCL);              /* exclusive access, best effort */
	fdSerial = fd;
	return fd;
}

void serialClose(void) {
	pthread_mutex_lock(&txLock);
	if (fdSerial >= 0) {
		close(fdSerial);
		fdSerial = -1;
	}
	pthread_mutex_unlock(&txLock);
}

int serialFd(void) {
	return fdSerial;
}

int serialSetBaud(unsigned baud) {
	int rc;

	if (fdSerial < 0) {
		errno = EBADF;
		return -1;
	}
	pthread_mutex_lock(&txLock);
	tcdrain(fdSerial);                      /* let pending bytes leave first */
	rc = serialConfigure(fdSerial, baud);
	pthread_mutex_unlock(&txLock);
	return rc;
}

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
		n = write(fdSerial, buf + done, len - done);
		if (n > 0) {
			done += (size_t)n;
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		if (n < 0 && errno == EAGAIN) {
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

void serialFlushInput(void) {
	if (fdSerial >= 0)
		tcflush(fdSerial, TCIFLUSH);
}