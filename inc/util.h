#ifndef LGR_UTIL_H
#define LGR_UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define util_arrset(arr, len, value) \
    for (ptrdiff_t i = 0; i < (len); (arr)[i++] = (value))

#define util_membersize(type, member) (sizeof ((type*)0)->member)

/* Check if file FP exists. */
bool util_fexists(const char* fp);

/*
 * Count the number of decimal digits in X. If NEGSIGN is true, consider
 * the negative sign to be a digit.
 */
int util_digits(intmax_t x, bool negsign);

/* Integral sum/max/min. */
intmax_t util_add(intmax_t x, intmax_t y);
intmax_t util_max(intmax_t x, intmax_t y);
intmax_t util_min(intmax_t x, intmax_t y);

/*
 * Convert an integral cent quantity to string, with two decimal places and
 * thousands separators. Does NOT write NUL at the end.
 * 
 * Parameters
 * ----------
 * x
 *      Integer to convert, representing a number of cents.
 * buf
 *      Stores the converted string. Assumed to have enough space.
 * 
 * Returns
 * -------
 * int
 *      The number of chars written.
 */
int util_fmtcents(intmax_t x, char* buf);

/* Return the number of chars needed to format X into a monetary string. */
int util_fmtcentslen(intmax_t x);

/*
 * Convert a decimal string to an integer.
 * 
 * Parameters
 * ----------
 * s
 *      The string to convert.
 * status
 *      If not NULL, `*status` stores true if successful and false
 *      otherwise.
 * 
 * Returns
 * -------
 * long long
 *      The converted number. 0 on error.
 */
long long util_stoi(const char* s, bool* status);

/*
 * Read a key=value line from an INI file. Spaces will not be trimmed.
 * 
 * Parameters
 * ----------
 * f
 *      Stream to read from.
 * bufsize
 *      Size of `buf`. No line must exceed (bufsize-2) chars.
 * buf
 *      Buffer to store the read line.
 * key
 *      Key string will be stored in `*key`.
 * value
 *      Value string will be stored in `*value`.
 * 
 * Returns
 * int
 *      -1: No more lines to read
 *      0: Success
 *      1: Line too long
 *      2: Equals sign not found
 */
int util_readiniline(FILE* f, int bufsize, char* buf, char** key, char** value);

#endif
