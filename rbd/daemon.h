/* *********************************************************************************** */
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

#ifndef daemon_h
#define daemon_h

/* *********************************************************************************** *
 * @brief Detach the process from its controlling terminal and run it as a daemon.
 *
 * The parent process exits after forking. The child creates a new session,
 * changes its working directory to /tmp, redirects standard input, output, and
 * error to /dev/null, writes its process ID to a locked file, and installs
 * handlers for SIGHUP, SIGTERM, and SIGINT.
 *
 * @param file Path to the PID file to create and lock.
 *
 * @note Exits the process with failure if daemon setup or PID-file handling fails.
 * *********************************************************************************** */
void daemonize(const char *pidFile);

#endif /* daemon_h */
