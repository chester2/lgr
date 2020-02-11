// <1> General
// <2> Shift
// <3> String Conversion

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "util.h"
#include "date.h"

/* Initialize int variables and store date components. */
#define DECOMPOSE(dt, y, m, d) \
    int y = dt_gety(dt); \
    int m = dt_getm(dt); \
    int d = dt_getd(dt)

/* Last day of month. Assume Y and M are valid. */
static int ldom(int y, int m)
{
    const int ldoms[] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31,
    };
    return (
        m != 2
        || y%4 != 0
        || (y%100 == 0 && y%400 != 0)
    ) ? ldoms[m-1] : 29;
}


// <1> General

int32_t dt_dt(int y, int m, int d)
{
    return (int32_t)y*10000 + m*100 + d;
}

int32_t dt_today(void)
{
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    return dt_dt(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
}

int dt_gety(int32_t dt) {return dt / 10000;}
int dt_getm(int32_t dt) {return dt / 100 % 100;}
int dt_getd(int32_t dt) {return dt % 100;}

int32_t dt_sety(int32_t dt, int y) {return dt % 10000 + y*10000;}
int32_t dt_setm(int32_t dt, int m) {return dt + 100*(m - dt_getm(dt));}
int32_t dt_setd(int32_t dt, int d) {return dt / 100 * 100 + d;}

bool dt_isy(intmax_t y) {return y >= 1 && y <= 9999;}
bool dt_ism(intmax_t m) {return m >= 1 && m <= 12;}
bool dt_isd(intmax_t d) {return d >= 1 && d <= 31;}
bool dt_isdt(int32_t dt)
{
    DECOMPOSE(dt, y, m, d);
    return dt_isy(y)
        && dt_ism(m)
        && d >= 1
        && d <= ldom(y, m);
}


// <2> Shift

int32_t dt_shifty(int32_t dt, int offset)
{
    DECOMPOSE(dt, y, m, d);
    y += offset;
    return dt_isy(y)
        ? dt_dt(y, m, util_min(d, ldom(y, m)))
        : 0;
}

int32_t dt_shiftm(int32_t dt, int offset)
{
    DECOMPOSE(dt, y, m, d);
    m += offset;
    if (m > 12) {
        y += (m - 1) / 12;
        if (!dt_isy(y)) return 0;
        m = (m - 1) % 12 + 1;
    } else if (m < 1) {
        y += (m - 12) / 12;
        if (!dt_isy(y)) return 0;
        m = (m % 12 + 11) % 12 + 1;
    }
    return dt_dt(y, m, util_min(d, ldom(y, m)));
}

int32_t dt_shiftd(int32_t dt, int offset)
{
    // From Howard Hinnant's chrono-compatible date algorithms. Convert
    // date components to an offset from 0000-03-01, shift days, then
    // convert that offset back to components.
    DECOMPOSE(dt, y, m, d);

    int yy = y - (m < 3);
    int mm = m + (m < 3 ? 9 : -3);
    int dd = d - 1;
    int era = (yy >= 0 ? yy : yy - 399) / 400;
    int yoe = yy - era*400;
    int doy = (153*mm + 2) / 5 + dd;
    int32_t doe = yoe*365 + yoe/4 - yoe/100 + doy;

    int32_t ts = era*146097 + doe + offset;
    if (ts < 306 || ts > 3652364) return 0;

    era = (ts >= 0 ? ts : ts + 1 - 146097) / 146097;
    doe = ts - era*146097;
    yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
    doy = doe - yoe*365 - yoe/4 + yoe/100;
    yy = yoe + era*400;
    mm = (5*doy + 2) / 153;
    dd = doy - (153*mm + 2) / 5;

    return dt_dt(yy + (mm >= 10), mm + (mm < 10 ? 3 : -9), dd + 1);
}


// <3> String Conversion

const char* dt_mmm(int m)
{
    static const char* const mmm[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
    };
    return mmm[m-1];
}

bool dt_isiso(const char* s)
{
    for (int i = 0; i < DT_ISOLEN; i++) {
        if (s[i] == '\0') return false;
        if (i == 4 || i == 7) {
            if (s[i] != '-') return false;
        } else if (s[i] < '0' || s[i] > '9') {
            return false;
        }
    }
    return s[DT_ISOLEN] == '\0';
}

int32_t dt_fromiso(const char* s)
{
    int y, m, d;
    sscanf(s, "%d-%d-%d", &y, &m, &d);
    return dt_dt(y, m, d);
}

char* dt_toiso(int32_t dt)
{
    static char s[DT_ISOLEN + 1];
    DECOMPOSE(dt, y, m, d);
    sprintf(s, "%04d-%02d-%02d", y, m, d);
    return s;
}

char* dt_fmt(int32_t dt)
{
    static char s[sizeof "mmm dd, yyyy"];
    DECOMPOSE(dt, y, m, d);
    sprintf(s, "%s %d, %04d", dt_mmm(m), d, y);
    return s;
}
