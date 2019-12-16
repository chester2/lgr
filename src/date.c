// <1> Date-Offset Conversion
// <2> Date-String Conversion
// <3> Offset-String Conversion

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "date.h"

#define DAYS_IN_ERA (400 * 365 + 100 - 3)
// #define DSTR_PARTS 3
// #define DSTR_LEN 11     // includes trailing '\0'


// <1> Date-Offset Conversion


// Convert a Date struct to an offset in days from 0000-03-01.
int32_t dt_d2o(Date* dp)
{
    int y = dp->year - (dp->month < 3);
    int m = dp->month + (dp->month < 3 ? 9 : -3);
    int d = dp->day - 1;

    int era = (y >= 0 ? y : y - 399) / 400;
    int yoe = y - era * 400;
    int32_t doy = (153 * m + 2) / 5 + d;
    int32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return DAYS_IN_ERA * era + doe;
}


// Date dt_o2d(int32_t o)
// {

// }


// <2> Date-String Conversion


/* Convert a Date struct to a string of the format yyyy-mm-dd and put the result in S.

Returns success status. */
bool dt_d2s(Date* dp, char* s)
{
    int const dstr_len = 11;
    int result = snprintf(
        s,
        dstr_len,
        "%04hd-%02hhd-%02hhd",
        dp->year,
        dp->month,
        dp->day
    );
    return dstr_len - 1 == result;
}


/* Convert a date string S of the format yyyy-mm-dd to a Date struct.

If STATUSP is not NULL, true or false is stored in *STATUSP depending on whether the conversion succeeded or not. */
Date dt_s2d(char* s, bool* statusp)
{
    // Check null byte.
    if (s[10]) {goto error;}
    for (size_t i = 0; i < 10; i++) {
        if (!s[i]) {goto error;}
    }

    char buf[5];
    char* endptr;
    long parts[3]; // will hold strtol results
    size_t part_lengths[] = {4, 2, 2};
    char* part_starts[] = {
        s,
        s + 4 + 1,
        s + 4 + 1 + 2 + 1
    };

    // strtol each date component
    for (size_t i = 0; i < 3; i++) {
        buf[part_lengths[i]] = '\0';
        memcpy(buf, part_starts[i], part_lengths[i]);
        errno = 0;
        parts[i] = strtol(buf, &endptr, 10);
        if (errno || *endptr) {goto error;}
    }
    if (statusp) {*statusp = true;}
    return (Date){parts[0], parts[1], parts[2]};

error:
    if (statusp) {*statusp = false;}
    Date date;
    return date;
}


// <3> Offset-String Conversion


/* Convert a date string S of the format yyyy-mm-dd to an offset in days from 0000-03-01.

If STATUSP is not NULL, 1 or 0 is stored in *STATUSP depending on whether the conversion succeeded or not. */
int32_t dt_s2o(char* s, bool* statusp)
{
    bool internal_status;
    Date date = dt_s2d(s, &internal_status);
    if (statusp) {*statusp = internal_status;}
    return internal_status ? dt_d2o(&date) : 0;
}

/* Convert an offset to a yyyy-mm-dd string and place in S.

Return true or false on success or failure respectively. */
// bool dt_o2s(int32_t o, char* s)
// {

// }