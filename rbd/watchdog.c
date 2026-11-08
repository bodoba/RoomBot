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
/* Copyright (c) 2026 by Bodo Bauer <bb@bb-zone.com> - GPL v3 or later */
#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <time.h>

#include "commands.h"
#include "logging.h"
#include "watchdog.h"

static pthread_mutex_t wdLock = PTHREAD_MUTEX_INITIALIZER;  /**< Protects the state below */
static pthread_cond_t wdCond;                               /**< Wakes the thread on arming */
static bool wdArmed;                                        /**< true = deadline is active */
static struct timespec wdDeadline;                          /**< Absolute CLOCK_MONOTONIC time */

/**
 * @brief Compute an absolute monotonic time that lies ms milliseconds ahead.
 *
 * @param ts Receives the deadline.
 * @param ms Offset from now in milliseconds.
 */
static void deadlineIn(struct timespec *ts, unsigned ms)
{
	clock_gettime(CLOCK_MONOTONIC, ts);
	ts->tv_sec += ms / 1000;
	ts->tv_nsec += (long)(ms % 1000) * 1000000L;
	if (ts->tv_nsec >= 1000000000L) {
		ts->tv_sec++;
		ts->tv_nsec -= 1000000000L;
	}
}

/**
 * @brief Check whether a monotonic deadline has passed.
 *
 * @param dl Deadline.
 * @return true if now >= dl.
 */
static bool deadlinePassed(const struct timespec *dl)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	if (now.tv_sec != dl->tv_sec)
		return now.tv_sec > dl->tv_sec;
	return now.tv_nsec >= dl->tv_nsec;
}

/**
 * @brief Watchdog thread: stops the robot when the deadline expires.
 *
 * @param arg Unused.
 * @return Never returns.
 */
static void *watchdogThread(void *arg)
{
	int rc;

	(void)arg;
#ifdef __linux__
	pthread_setname_np(pthread_self(), "rbd_watchdog");
#endif
	pthread_mutex_lock(&wdLock);
	for (;;) {
		if (!wdArmed) {
			/* Nothing to watch: sleep until watchdogFeed() arms us. */
			pthread_cond_wait(&wdCond, &wdLock);
			continue;
		}
		rc = pthread_cond_timedwait(&wdCond, &wdLock, &wdDeadline);

		/*
		 * Re-check everything under the lock: the deadline may have
		 * been moved or the watchdog disarmed while we were waking up.
		 */
		if (rc != ETIMEDOUT || !wdArmed || !deadlinePassed(&wdDeadline))
			continue;

		writeLog(LOG_WARNING, "Watchdog: no drive command for %d ms, stopping",
		    WATCHDOG_TIMEOUT_MS);
		/* The lock is held, so no new command can be armed meanwhile. */
		if (commandDrive(0, DRIVE_RADIUS_STRAIGHT) < 0) {
			writeLog(LOG_ERR, "Watchdog: stop failed (%s), retrying",
			    strerror(errno));
			deadlineIn(&wdDeadline, WATCHDOG_RETRY_MS);  /* stay armed */
		} else {
			wdArmed = false;
		}
	}
	return NULL;
}

int watchdogStart(void)
{
	pthread_condattr_t attr;
	pthread_t tid;
	int rc;

	/* The condition variable must use the same clock as our deadlines. */
	pthread_condattr_init(&attr);
	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
	pthread_cond_init(&wdCond, &attr);
	pthread_condattr_destroy(&attr);

	rc = pthread_create(&tid, NULL, watchdogThread, NULL);
	if (rc != 0) {
		errno = rc;
		return -1;
	}
	return pthread_detach(tid);
}

void watchdogFeed(void)
{
	bool wasArmed;

	pthread_mutex_lock(&wdLock);
	wasArmed = wdArmed;
	deadlineIn(&wdDeadline, WATCHDOG_TIMEOUT_MS);
	wdArmed = true;
	pthread_mutex_unlock(&wdLock);

	/*
	 * Only a sleeping (disarmed) thread needs a wake-up. An armed thread
	 * wakes at the old deadline, sees the new one and waits again.
	 */
	if (!wasArmed)
		pthread_cond_signal(&wdCond);
}

void watchdogDisarm(void)
{
	pthread_mutex_lock(&wdLock);
	wdArmed = false;
	pthread_mutex_unlock(&wdLock);
}