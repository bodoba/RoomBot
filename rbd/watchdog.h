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
#ifndef WATCHDOG_H
#define WATCHDOG_H

#define WATCHDOG_TIMEOUT_MS  300    /**< Stop the robot after this time without a command */
#define WATCHDOG_RETRY_MS    100    /**< Retry interval if the stop command fails         */

/**
 * @brief Start the watchdog thread.
 *
 * Call it after signalsInit(), so the thread inherits the blocked signal
 * mask, and after daemonize(), because fork() does not copy threads.
 *
 * @return 0 on success, -1 on error (errno set).
 */
int watchdogStart(void);

/**
 * @brief Arm the watchdog or postpone its deadline.
 *
 * Call it for every drive command that moves the robot, before sending the
 * command to the Roomba. If no further call follows within
 * WATCHDOG_TIMEOUT_MS, the thread stops the robot.
 */
void watchdogFeed(void);

/**
 * @brief Disarm the watchdog.
 *
 * Call it for a stop command, before sending the command to the Roomba.
 * The watchdog then does nothing until the next watchdogFeed().
 */
void watchdogDisarm(void);

#endif /* WATCHDOG_H */