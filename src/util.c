/* Generic utilities. */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

bool util_fexists(const char* fp)
{
    return !access(fp, F_OK);
}

int util_digits(intmax_t x, bool negsign)
{
    if (x == 0) return 1;
    int count = 1 + (negsign && (x < 0));
    for (; (x /= 10); count++);
    return count;
}

intmax_t util_add(intmax_t x, intmax_t y)
{
    return x + y;
}

intmax_t util_max(intmax_t x, intmax_t y)
{
    return x > y ? x : y;
}

intmax_t util_min(intmax_t x, intmax_t y)
{
    return x < y ? x : y;
}

int util_fmtcents(intmax_t x, char* buf)
{
    bool neg = x < 0;
    intmax_t mag = neg ? -x : x;

    buf[0] = mag % 10 + '0';
    buf[1] = (mag /= 10) % 10 + '0';
    buf[2] = '.';
    buf[3] = (mag /= 10) % 10 + '0';

    int i = 4;
    while ((mag /= 10)) {
        if ((i - 2) % 4 == 0) buf[i++] = ',';
        buf[i++] = mag % 10 + '0';
    }

    if (neg) buf[i++] = '-';
    for (int j = 0, k = i - 1; j < k; j++, k--) {
        char tmp = buf[j];
        buf[j] = buf[k];
        buf[k] = tmp;
    }
    return i;
}

int util_fmtcentslen(intmax_t x)
{
    int len = util_max(3, util_digits(x, false))
        + (x < 0)
        + 1; // decimal
    x /= 100;
    while ((x /= 1000)) len++;
    return len;
}

long long util_stoi(const char* s, bool* status)
{
    bool _;
    if (status == NULL) status = &_;
    errno = 0;
    char* endptr;
    long long out = strtoll(s, &endptr, 10);
    return (*status = (*s && !errno && !*endptr)) ? out : 0;
}

int util_readiniline(FILE* f, int bufsize, char* buf, char** key, char** value)
{
    if (NULL == fgets(buf, bufsize, f))
        return -1;

    int len = strlen(buf);
    if (len == bufsize - 1 && buf[len-1] != '\n')
        return 1;
    if (buf[len-1] == '\n')
        buf[len-1] = '\0';

    char* eq = strchr(buf, '=');
    if (eq == NULL)
        return 2;

    *eq = '\0';
    *key = buf;
    *value = eq + 1;
    return 0;
}
