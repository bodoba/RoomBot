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
#include <stdlib.h>

#ifndef JSON_H
#define JSON_H

/**
 * @brief Find the value that belongs to a key in a flat JSON object.
 *
 * Minimal helper for the simple payloads of this module. It does not handle
 * nested objects with equal key names.
 *
 * @param json NUL terminated JSON text.
 * @param key  Key name without quotes.
 *
 * @return Pointer to the first character of the value, or NULL if not found.
 */
const char *findValue(const char *json, const char *key);

/**
 * @brief Read an integer value from a flat JSON object.
 *
 * @param json NUL terminated JSON text.
 * @param key  Key name without quotes.
 * @param out  Receives the value.
 *
 * @return 0 on success, -1 if the key is missing or not a number.
 */
int jsonInt(const char *json, const char *key, long *out);

/**
 * @brief Read a string value from a flat JSON object.
 *
 * Backslash escapes are reduced to the escaped character.
 *
 * @param json NUL terminated JSON text.
 * @param key  Key name without quotes.
 * @param out  Output buffer.
 * @param size Size of the output buffer.
 *
 * @return 0 on success, -1 if the key is missing or not a string.
 */
int jsonString(const char *json, const char *key, char *out, size_t size);

#endif /* JSON_H */