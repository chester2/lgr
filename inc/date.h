#ifndef LGR_DATE_H
#define LGR_DATE_H

#include <stdbool.h>
#include <stdint.h>

#define DT_ISOLEN ((int)sizeof("yyyy-mm-dd") - 1)

/* Create a (possibly invalid) date. */
int32_t dt_dt(int y, int m, int d);

/* Return the current local date. */
int32_t dt_today(void);

/* Valid date component getters. */
int dt_gety(int32_t dt);
int dt_getm(int32_t dt);
int dt_getd(int32_t dt);

/* Valid date component setters. Returned date may be invalid. */
int32_t dt_sety(int32_t dt, int y);
int32_t dt_setm(int32_t dt, int m);
int32_t dt_setd(int32_t dt, int d);

/* Date and component validations. */
bool dt_isy(intmax_t y);
bool dt_ism(intmax_t m);
bool dt_isd(intmax_t d);
bool dt_isdt(int32_t dt);

/* Shift valid date using component. Returned date may be invalid. For
shift year and shift month, if the resultant day component is too large, it
will be set to the last day of month. */
int32_t dt_shifty(int32_t dt, int offset);
int32_t dt_shiftm(int32_t dt, int offset);
int32_t dt_shiftd(int32_t dt, int offset);

/* Return a read-only string to M's short month name. */
const char* dt_mmm(int m);

/* Check if S is of the form "yyyy-mm-dd". */
bool dt_isiso(const char* s);

/* Convert a string of the form "yyyy-mm-dd" to a (possibly invalid) date. */
int32_t dt_fromiso(const char* s);

/* Return the ISO string representation of a valid date. The return value
is a statically allocated string. */
char* dt_toiso(int32_t dt);

/* Return the pretty-formatted string representation of a valid date. The
return value is a statically allocated string. */
char* dt_fmt(int32_t dt);

#endif
