#include <assert.h>
#include <limits.h>
#include <stdint.h>

#include "program.h"
#include "t_framework.h"

void test_parsey(void);
void test_parsedt(void);

int main(int argc, char** argv)
{
    TFWK_LOG = (argc > 1 && STR_EQ(argv[1], "-s"));
    test_parsey();
    test_parsedt();
}


// Parse Year

void assert_parsey(const char* s, int expected)
{
    int y;
    if (expected == INT_MAX) {
        assert(!prog_parsey(s, &y));
        log_cycle("%s: --", s, expected);
    } else {
        assert(prog_parsey(s, &y));
        assert(y == expected);
        log_cycle("%s: %d", s, expected);
    }
}

void test_parsey(void)
{
    log_intro("parsey");

    int this_year = dt_gety(dt_today());
    assert_parsey("y", this_year);
    assert_parsey("y0", this_year);
    assert_parsey("y+0", this_year);
    assert_parsey("y-0", this_year);
    assert_parsey("y1", this_year + 1);
    assert_parsey("y+1", this_year + 1);
    assert_parsey("y-24", this_year - 24);
    assert_parsey("y+9999", this_year + 9999);
    assert_parsey("10000", INT_MAX);
    assert_parsey("0", INT_MAX);

    log_end();
}


// Parse Date

void assert_parsedt(const char* s, int32_t expected)
{
    int dt;
    if (expected == INT32_MAX) {
        assert(!prog_parsedt(s, &dt));
        log_cycle("%s: --", s, expected);
    } else {
        assert(prog_parsedt(s, &dt));
        assert(dt == expected);
        log_cycle("%s: %d", s, expected);
    }
}

void test_parsedt(void)
{
    log_intro("parsedt");

    int32_t today = dt_today();
    assert_parsedt("d", today);
    assert_parsedt("d0", today);
    assert_parsedt("d+0", today);
    assert_parsedt("d-0", today);
    assert_parsedt("d-", INT32_MAX);
    assert_parsedt("d4", dt_shiftd(today, 4));
    assert_parsedt("d-900", dt_shiftd(today, -900));
    assert_parsedt("0001-01-01", 10101);
    assert_parsedt("2000-02-28", 20000228);
    assert_parsedt("2000-2-28", INT32_MAX);
    assert_parsedt  ("1-01-01", INT32_MAX);

    log_end();
}
