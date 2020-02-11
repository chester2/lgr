// <1> Max / Min
// <2> Digits

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include "util.h"
#include "t_framework.h"

void test_maxmin(void);
void test_digits(void);

int main(int argc, char** argv)
{
    TFWK_LOG = (argc > 1 && STR_EQ(argv[1], "-s"));
    test_maxmin();
    test_digits();
}


// <1> Max / Min

void assert_maxmin(intmax_t larger, intmax_t smaller)
{
    log_cycle("%" PRIdMAX " > %" PRIdMAX, larger, smaller);
    assert(util_max(larger, smaller) == larger);
    assert(util_min(larger, smaller) == smaller);
}

void test_maxmin(void)
{
    log_intro("max/min");
    assert_maxmin(123, 1);
    assert_maxmin(96, -1023);
    assert_maxmin(INT32_MAX, INT32_MIN);
    log_end();
}


// <2> Digits

void assert_digits(intmax_t x, bool negsign, int expected)
{
    log_cycle("%" PRIdMAX, x);
    assert(util_digits(x, negsign) == expected);
}

void test_digits(void)
{
    log_intro("digits");
    assert_digits(0, false, 1);
    assert_digits(-0, true, 1);
    assert_digits(1082841, false, 7);
    assert_digits(1082841, true, 7);
    assert_digits(-1082841, false, 7);
    assert_digits(-1082841, true, 8);
    assert_digits(INT32_MAX, true, 10);
    assert_digits(INT32_MIN, true, 11);
    log_end();
}
