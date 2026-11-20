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

#include "logging.h"
#include <syslog.h>
#include <string.h>

/* *********************************************************************************** *
 * Default settings for logging
 * *********************************************************************************** */
static int  logLevel  = LOG_ERR;
static bool useSyslog = true;

/* *********************************************************************************** *
 * Textual representation of log levels for console output
 * *********************************************************************************** */
static const char * logLevelText[] = {  "emergency",
                                        "alert",
                                        "critical",
                                        "error",
                                        "warning",
                                        "notice",
                                        "info",
                                        "debug"
};

static int normalizeLogLevel( int level ) {
    if ( level < LOG_EMERG ) {
        level = LOG_EMERG;
    } else if ( level > LOG_DEBUG ) {
        level = LOG_DEBUG;
    }

    return level;
}

static bool validLogLevel( int level ) {
    return ( level >= LOG_EMERG ) && ( level <= LOG_DEBUG );
}

/* *********************************************************************************** *
 *    P U B L I C  I N T E R F A C E
 * *********************************************************************************** */

void initLog( bool syslog ) {
    useSyslog = syslog;
    
    if ( useSyslog) {
        openlog(NULL, LOG_PID, LOG_USER);            // use syslog to create a trace
    }
}

int setLogLevel( int level ) {
    logLevel = normalizeLogLevel(level);
    writeLog(LOG_NOTICE, "Set log level to %s", logLevelText[logLevel]);
    return logLevel;
}
    
int getLogLevel( void ) {
    return logLevel;
}

void writeLog( int level, const char* format,...) {
    va_list valist;
    if ( validLogLevel(level) && ( level <= logLevel ) ) {
        if ( useSyslog ) {
            va_start(valist, format);
            vsyslog(level, format, valist);
            va_end(valist);
        } else {
            time_t now = time(NULL);
            struct tm timestamp;

            if ( ( now != (time_t)-1 ) &&
                 ( localtime_r(&now, &timestamp) != NULL ) ) {
                fprintf(stdout,
                        "%04d-%02d-%02d %02d:%02d:%02d <%s> ",
                        timestamp.tm_year + 1900,
                        timestamp.tm_mon + 1,
                        timestamp.tm_mday,
                        timestamp.tm_hour,
                        timestamp.tm_min,
                        timestamp.tm_sec,
                        logLevelText[level]);
            } else {
                fprintf(stdout, "<%s> ", logLevelText[level]);
            }

            va_start(valist, format);
            vprintf(format, valist);
            va_end(valist);
            fputc('\n', stdout);
        }
    }
}
