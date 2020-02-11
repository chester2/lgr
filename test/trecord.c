#include <assert.h>
#include <string.h>

#include "date.h"
#include "record.h"
#include "t_framework.h"

void test_general(void);

int main(int argc, char** argv)
{
    TFWK_LOG = (argc > 1 && STR_EQ(argv[1], "-s"));
    test_general();
}

void assert_general(const char* s, Record* rec)
{
    log_cycle("%p: %s", (void*)rec, s);
    Record record;
    if (rec == NULL) {
        assert(!rec_fromstr(&record, s));
        return;
    }
    assert(rec_fromstr(&record, s));
    assert(record.amt == rec->amt);
    assert(record.dt == rec->dt);
    assert(STR_EQ(record.cat, rec->cat));
    assert(STR_EQ(record.desc, rec->desc));
    assert(STR_EQ(s, rec_tostr(&record)));
}

void test_general(void)
{
    log_intro("general");
    Record rec;
    assert_general(
        "2020-01-03\t4000\tmisc\t",
        rec_init(&rec, 20200103, 4000, "misc", "")
    );
    assert_general(
        "2020-01-03\t-4000\tmisc\tstuff",
        rec_init(&rec, 20200103, -4000, "misc", "stuff")
    );
    assert_general("2020-01-3\t1\ta\t", NULL); // bad ts
    assert_general("2020-01-03\t\ta\t", NULL); // empty amt not allowed
    assert_general("2020-01-03\t0\ta\t", NULL); // amt 0 not allowed
    assert_general("2020-01-03\t1\t\t", NULL); // empty cat not allowed
    assert_general("2020-01-03\t4000\tmisc", NULL); // not enough delimiters
    log_end();
}
