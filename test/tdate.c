#include <assert.h>

#include "date.h"
#include "t_framework.h"

void test_general(void);
void test_shift(void);
void test_str(void);

int main(int argc, char** argv)
{
    TFWK_LOG = (argc > 1 && STR_EQ(argv[1], "-s"));
    test_general();
    test_shift();
    test_str();
}

void test_general(void)
{
    log_intro("general");
    assert(dt_dt(1, 2, 3) == 10203);
    assert(dt_getd(19991204) == 4);
    assert(dt_setm(19991231, 2) == 19990231);
    assert(dt_isdt(19991231));
    assert(!dt_isdt(19990231));
    log_end();
}

void test_shift(void)
{
    log_intro("shift");

    assert(dt_shifty(10101, 1) == 20101);
    assert(dt_shifty(10101, 0) == 10101);
    assert(dt_shifty(20000229, 1) == 20010228);
    assert(dt_shifty(20000228, 1) == 20010228);
    assert(!dt_isdt(dt_shifty(10101, -1)));

    assert(dt_shiftm(99991231, 0) == 99991231);
    assert(!dt_isdt(dt_shiftm(99991231, 1)));
    assert(dt_shiftm(20000131, 25) == 20020228);
    assert(dt_shiftm(20000131, -25) == 19971231);

    assert(dt_shiftd(10101, 0) == 10101);
    assert(!dt_isdt(dt_shiftd(10101, -1)));
    assert(dt_shiftd(99991231, -300) == 99990306);

    log_end();
}

void test_str(void)
{
    log_intro("string");

    assert(STR_EQ(dt_mmm(1), "Jan"));
    assert(STR_EQ(dt_mmm(6), "Jun"));
    assert(STR_EQ(dt_mmm(12), "Dec"));

    assert(dt_isiso("1234-56-78"));
    assert(!dt_isiso("1234-565-78"));
    assert(!dt_isiso("1234-56-7"));
    assert(!dt_isiso("a234-56-78"));
    assert(!dt_isiso("1234056-78"));

    assert(dt_fromiso("1234-01-23") == 12340123);
    assert(dt_fromiso("2000-02-29") == 20000229);
    assert(!dt_isdt(dt_fromiso("2001-02-29")));

    assert(STR_EQ(dt_toiso(10101), "0001-01-01"));
    assert(STR_EQ(dt_toiso(10101), "0001-01-01"));
    assert(STR_EQ(dt_fmt(10101), "Jan 1, 0001"));
    assert(STR_EQ(dt_fmt(10110), "Jan 10, 0001"));

    log_end();
}
