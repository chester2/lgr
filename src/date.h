#ifndef DATE_H
#define DATE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    short year;
    signed char month;
    signed char day;
} Date;

// Converters
int32_t dt_d2o(Date* dp);
// Date dt_o2d(int32_t o);
bool dt_d2s(Date* dp, char* s);
Date dt_s2d(char* s, bool* statusp);
int32_t dt_s2o(char* s, bool* statusp);
// bool dt_o2s(int32_t o, char* s);

#endif