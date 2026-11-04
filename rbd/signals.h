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
#ifndef SIGNALS_H
#define SIGNALS_H

/**
 * @brief Block SIGINT, SIGTERM and SIGHUP and ignore SIGPIPE.
 *
 * Call this in main() before any other thread is created, so that all
 * threads inherit the blocked signal mask.
 *
 * @return 0 on success, -1 on error (errno set).
 */
int signalsInit(void);

/**
 * @brief Start the signal handling thread.
 *
 * The thread waits for SIGINT/SIGTERM and then powers down the Roomba,
 * calls the shutdown hook and exits the process. SIGHUP is only logged.
 * Call this after daemonize(), because fork() does not copy threads.
 *
 * @param hook Called after the power-down and before exit(), may be NULL.
 *             Typical use: stop MQTT and remove the PID file.
 *
 * @return 0 on success, -1 on error (errno set).
 */
int signalsStart(void (*hook)(void));

#endif /* SIGNALS_H */