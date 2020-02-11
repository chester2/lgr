#include <assert.h>
#include <inttypes.h>
#include <string.h>

#include "hashtable.h"
#include "t_framework.h"

void test_general(void);

int main(int argc, char** argv)
{
    TFWK_LOG = (argc > 1 && STR_EQ(argv[1], "-s"));
    test_general();
}

void assert_insert(HashTable* ht, const void* key, int64_t value)
{
    if (ht_ktype(ht) == HT_INT)
        log_cycle("%" PRId64 ": %" PRId64, *(const int64_t*)key, value);
    else
        log_cycle("%s: %" PRId64, (const char*)key, value);
    bool exists = ht_get(ht, key);
    int64_t origv = exists ? *ht_get(ht, key) : 0;
    int64_t* v = ht_insert(ht, key, value);
    assert(*v == (exists ? origv : value));
}

void test_general(void)
{
    log_intro("insert");
    HashTable* ht = ht_new(HT_STR);

    assert_insert(ht, "a", 1);
    assert_insert(ht, "a", 2);
    assert_insert(ht, "b", 10);
    assert_insert(ht, "c", 100);
    assert_insert(ht, "d", 1000);
    assert_insert(ht, "e", 10000);
    assert_insert(ht, "f", 100000);
    assert_insert(ht, "z", 1000000);
    assert_insert(ht, "y", 10000000);
    assert(!ht_delete(ht, "m"));
    assert(ht_delete(ht, "f"));
    assert_insert(ht, "x", 100000000);

    assert(ht_sum(ht) == 111011111);
    assert(ht_max(ht) == 100000000);
    assert(ht_min(ht) == 1);

    void test_sort(HashTable* ht);
    test_sort(ht);

    ht_free(ht);
    log_end();
}

void test_sort(HashTable* ht)
{
    log_end();
    log_intro("sort");

    ht_sort(ht, false, false);
    const void* key;
    int64_t value;
    ht_foreach(ht, key, &value) {
        if (TFWK_LOG) {
            if (ht_ktype(ht) == HT_INT)
                log_cycle("%" PRId64 ": %" PRId64, *(const int64_t*)key, value);
            else
                log_cycle("%s: %" PRId64, (const char*)key, value);
        }
    }
}