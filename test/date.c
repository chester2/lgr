// <1> Test Helpers
// <2> Tests
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "date.h"

#define NAME "date"


static void seperate_stdout(void (*test)(void), char* funcname);
static void test_d2o(void);
// static void test_o2d(void);
// static void test_d2s(void);
// static void test_s2d(void);
// static void test_s2o(void);
// static void test_o2s(void);


int main(void)
{
    seperate_stdout(&test_d2o, "d2o");
}


// <1> Test Helpers


// Print stuff before/after test's output.
static void seperate_stdout
(void (*test)(void), char* funcname)
{
    char divider[] = "------------------------\n";
    printf(divider);
    printf(NAME " - %s\n", funcname);
    printf(divider);
    test();
    printf("\n");
}


// Construct Date using the given components and return the struct's pointer.
static Date*
quickdp(short y, signed char m, signed char d)
{
    static Date date;
    date.year = y;
    date.month = m;
    date.day = d;
    return &date;
}


// <2> Tests


static void test_d2o(void)
{
    int date_parts[][3] = {
        {0, 3, 1},
        {0, 2, 29},
        {0, 3, 2}
    };
    int32_t expected[] = {0, -1, 1};
    size_t cases = sizeof expected / sizeof expected[0];

    for (size_t i = 0; i < cases; i++) {
        int32_t result = dt_d2o(quickdp(
            date_parts[i][0],
            date_parts[i][1],
            date_parts[i][2]
        ));
        if (expected[i] == result) {
            printf("O | i = %zu\n", i);
        } else {
            printf(
                "X | i = %zu | result %" PRId32 ", expected %" PRId32 "\n",
                i, result, expected[i]
            );
        }
    }
}

