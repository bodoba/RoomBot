/* *********************************************************************************** */
/*                                                                                     */
/*  Copyright (c) 2018 by Bodo Bauer <bb@bb-zone.com>                                  */
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
#ifndef logging_h
#define logging_h

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <syslog.h>

/* *********************************************************************************** *
 * The log levels are defined in syslog.h
 *   LOG_EMERG    A panic condition was reported to all processes.
 *   LOG_ALERT    A condition that should be corrected immediately.
 *   LOG_CRIT     A critical condition.
 *   LOG_ERR      An error message.
 *   LOG_WARNING  A warning message.
 *   LOG_NOTICE   A condition requiring special handling.
 *   LOG_INFO     A general information message.
 *   LOG_DEBUG    A message useful for debugging programs.
 * *********************************************************************************** */

/* *********************************************************************************** *
 * @brief Initialize logging and select the log output destination.
 *
 * @param syslog If true, send log messages to the system logger; otherwise, write
 *               them to standard output.
 * *********************************************************************************** */
void initLog( bool syslog );

/* ************************************************************************************
 * @brief Set the maximum severity level of messages to log.
 *
 * @param level The syslog severity level to use.
 * @return The effective log level after applying the supported upper limit.
 * *********************************************************************************** */
int setLogLevel( int level );
    
/* *********************************************************************************** *
 * @brief Get the currently configured log level.
 *
 * @return The current syslog severity threshold.
 * *********************************************************************************** */
int getLogLevel( void );

/* *********************************************************************************** *
 * @brief Write a formatted message if its severity is within the configured threshold.
 *
 * Messages are sent to the system logger or standard output, depending on the
 * destination selected by initLog().
 *
 * @param level The syslog severity level of the message.
 * @param format A printf-style format string.
 * @param ... Values referenced by the format string.
 * *********************************************************************************** */
void writeLog( int level, const char* format,...);

#endif /* logging_h */
