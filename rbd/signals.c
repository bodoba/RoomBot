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
#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "commands.h"
#include "serial.h"
#include "signals.h"

static void (*shutdownHook)(void);         /**< Optional cleanup callback */

/**
 * @brief Fill a signal set with the signals handled by this module.
 *
 * @param set Signal set to initialise.
 */
static void buildSet(sigset_t *set) {
	sigemptyset(set);
	sigaddset(set, SIGINT);
	sigaddset(set, SIGTERM);
	sigaddset(set, SIGHUP);
}

/**
 * @brief Power down the Roomba, clean up and terminate the process.
 *
 * Runs in thread context, so syslog() and mutexes are allowed.
 */
static void shutdownDaemon(void) {
	if (commandPowerDown() < 0)
		syslog(LOG_ERR, "Roomba power down failed: %s", strerror(errno));
	if (shutdownHook != NULL)
		shutdownHook();
	serialClose();
	closelog();
	exit(EXIT_SUCCESS);
}

/**
 * @brief Signal thread: waits synchronously for signals.
 *
 * @param arg Unused.
 * @return NULL if sigwait() fails.
 */
static void *signalThread(void *arg) {
	sigset_t set;
	int sig, rc;

	(void)arg;
#ifdef __linux__
	pthread_setname_np(pthread_self(), "rbd_signals");
#endif
	buildSet(&set);

	for (;;) {
		rc = sigwait(&set, &sig);
		if (rc != 0) {
			syslog(LOG_ERR, "sigwait failed: %s", strerror(rc));
			return NULL;
		}
		switch (sig) {
		case SIGHUP:
			syslog(LOG_WARNING, "Received SIGHUP signal.");
			break;
		case SIGINT:
		case SIGTERM:
			syslog(LOG_INFO, "Received %s, daemon exiting",
			    strsignal(sig));
			shutdownDaemon();       /* does not return */
			break;
		default:
			syslog(LOG_WARNING, "Unhandled signal %s", strsignal(sig));
			break;
		}
	}
	return NULL;
}

int signalsInit(void) {
	sigset_t set;
	struct sigaction sa;

	buildSet(&set);
    int rc = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (rc != 0) {
        errno = rc;
        return -1;
    }
    memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	return sigaction(SIGPIPE, &sa, NULL);
}

int signalsStart(void (*hook)(void)) {
	pthread_t tid;
	int rc;

	shutdownHook = hook;
	rc = pthread_create(&tid, NULL, signalThread, NULL);
	if (rc != 0) {
		errno = rc;
		return -1;
	}
	return pthread_detach(tid);
}