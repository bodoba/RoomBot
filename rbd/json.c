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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "json.h"

const char *findValue(const char *json, const char *key) {
    char pattern[32];
    const char *p;

    if (snprintf(pattern, sizeof(pattern), "\"%s\"", key) >= (int)sizeof(pattern))
        return NULL;
    p = strstr(json, pattern);
    if (p == NULL)
        return NULL;
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t')
        p++;
    if (*p != ':')
        return NULL;
    p++;
    while (*p == ' ' || *p == '\t')
        p++;
    return p;
}

int jsonInt(const char *json, const char *key, long *out) {
    const char *p = findValue(json, key);
    char *end;

    if (p == NULL)
        return -1;
    errno = 0;
    *out = strtol(p, &end, 10);
    if (end == p || errno != 0)
        return -1;
    return 0;
}

int jsonString(const char *json, const char *key, char *out, size_t size) {
    const char *p = findValue(json, key);
    size_t n = 0;

    if (p == NULL || *p != '"' || size == 0)
        return -1;
    p++;
    while (*p != '\0' && *p != '"' && n + 1 < size) {
        if (*p == '\\' && p[1] != '\0')
            p++;
        out[n++] = *p++;
    }
    out[n] = '\0';
    return 0;
}
