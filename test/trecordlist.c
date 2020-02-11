#include <assert.h>
#include <sys/stat.h>

#include "date.h"
#include "recordlist.h"
#include "t_framework.h"
#include "t_refrecs.h"

void test_getters(FILE* f);
void test_write(FILE* f);
void test_slice(FILE* f);
void test_insdel(FILE* f);
void test_filter(FILE* f);

int main(int argc, char** argv)
{
    TFWK_LOG = (argc > 1 && STR_EQ(argv[1], "-s"));

    // file setup
    FILE* f = ref_mkfile();

    test_getters(f);
    test_write(f);
    test_slice(f);
    test_insdel(f);
    test_filter(f);

    // teardown
    ref_rmfile(f);
}


// Getters

void test_getters(FILE* f)
{
    log_intro("getters");
    rl_init(f);
    assert(rl_count() == 10);
    assert(rl_slicestart() == 0);
    assert(rl_slicestop() == 10);
    assert(rl_slicecount() == 10);
    rl_deinit();
    log_end();
}


// Write

void test_write(FILE* f)
{
    log_intro("write");
    rl_init(f);

    char* fn = "_testelist_serialization.txt";

    FILE* newf = fopen(fn, "w");
    rl_write(newf);
    fclose(newf);

    // reference serialization file should be identical to
    // reserialized file
    struct stat ref, justwritten;
    stat(REF_FN, &ref);
    stat(fn, &justwritten);
    log_cycle("%ld %ld", ref.st_size, justwritten.st_size);
    assert(ref.st_size == justwritten.st_size);

    remove(fn);
    rl_deinit();
    log_end();
}


// Slice

void assert_slice(int32_t dt0, int32_t dt1, ptrdiff_t l, ptrdiff_t r)
{
    log_cycle("(%s, %s): (%d, %d)", dt_toiso(dt0), dt_toiso(dt1), l, r);
    rl_slice(dt0, dt1);
    assert(rl_slicestart() == l);
    assert(rl_slicestop() == r);
}

void test_slice(FILE* f)
{
    log_intro("slice");
    rl_init(f);

    assert_slice(19990101, 19990101, 0, 3);
    assert_slice(10101, 19990101, 0, 3);
    assert_slice(10101, 99991231, 0, 10);
    assert_slice(19990131, 19991230, 3, 5);

    rl_deinit();
    log_end();
}


// Insert Delete

void test_insdel(FILE* f)
{
    log_intro("insdel");
    rl_init(f);

    Record rec = {.dt=20051024, .amt=10001, .cat="gas", .desc="diesel"};
    rl_slice(20100102, 20100102);
    const Record* inserted = rl_insert(&rec);
    assert(inserted == rl_get(7));
    assert(rl_slicestart() == 8);
    assert(rl_slicestop() == 9);
    assert(rl_count() == 11);
    rl_delete(7);
    assert(rl_slicestart() == 7);
    assert(rl_slicestop() == 8);
    assert(rl_count() == 10);
    assert(!rl_delete(10));
    assert(!rl_delete(-1));
    assert(rl_delete(9));

    rl_deinit();
    log_end();
}


// Filter

void assert_filter(FILE* f, const char* pat, ptrdiff_t count)
{
    log_cycle("%s: %td", pat, count);
    rl_init(f);
    assert(count == rl_filtercat(pat, ','));
    rl_deinit();
}

void test_filter(FILE* f)
{
    log_intro("filter");
    assert_filter(f, "ab", 3);
    assert_filter(f, "ab,xy", 6);
    assert_filter(f, "c,ef", 7);
    assert_filter(f, "c,ef,YZ", 10);
    assert_filter(f, "", 10);
    assert_filter(f, ",,", 10);

    rl_init(f);
    assert(3 == rl_filterdesc(" ,ATE,leh", ','));
    rl_deinit();

    log_end();
}
