// <1> Initialization
// <2> Search
// <3> Insertion
// <4> General
// <5> Sort

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "hashtable.h"

#define DEFAULT_CAP 8
#define LOAD_FACTOR 0.5
#define IND_EMPTY (-1)
#define IND_DELETED (-2)

typedef struct {
    union {
        int64_t i;
        char s[32];
    } key;
    int64_t value;
    uint64_t hash;
    bool usable;
} Item;

struct hashtable {
    ptrdiff_t* indices;     // bucket; array of item array indices
    ptrdiff_t ind_cap;         // bucket capacity
    ptrdiff_t ind_usable;      // number of nonempty, nondeleted indices
    ptrdiff_t ind_deleted;     // number of deleted indices
    Item* items;            // item array
    ptrdiff_t item_cap;      // item array capacity
    ptrdiff_t item_next;     // next available item array slot
    ptrdiff_t next;         // next iteration item array slot
    enum ht_ktype ktype;    // key type
};


// <1> Initialization

HashTable* ht_new(enum ht_ktype ktype)
{
    HashTable* ht = malloc(sizeof(*ht));
    ptrdiff_t* indices = malloc(DEFAULT_CAP * sizeof(*indices));
    Item* items = malloc(DEFAULT_CAP * sizeof(*items));
    if (ht && indices && items) {
        util_arrset(indices, DEFAULT_CAP, IND_EMPTY);
        ht->indices = indices;
        ht->ind_cap = DEFAULT_CAP;
        ht->ind_usable = 0;
        ht->ind_deleted = 0;
        ht->items = items;
        ht->item_cap = DEFAULT_CAP;
        ht->item_next = 0;
        ht->next = 0;
        ht->ktype = ktype;
        return ht;
    }
    free(ht);
    free(indices);
    free(items);
    return NULL;
}

void ht_free(HashTable* ht)
{
    if (ht) {
        free(ht->indices);
        free(ht->items);
        free(ht);
    }
}


// <2> Search

static uint64_t gethash(enum ht_ktype ktype, const void* key)
{
    if (ktype == HT_INT) {
        return *(const int64_t*)key;
    } else if (ktype == HT_STR) {
        // FNV-1a string hash
        uint64_t hash = 14695981039346656037UL;
        const char* s = key;
        for (
            int i = 0
            ; s[i] && i < (int)(util_membersize(Item, key.s) - 1)
            ; i++
        ) {
            hash ^= (uint8_t)s[i];
            hash *= 1099511628211UL;
        }
        return hash;
    } else {
        return 0;
    }
}

static bool keysmatch(
    uint64_t hash,
    enum ht_ktype ktype,
    const void* key,
    const Item* item
) {
    if (hash != item->hash) return false;
    if (ktype == HT_INT)
        return true;
    else if (ktype == HT_STR)
        return 0 == strcmp(key, item->key.s);
    else
        return false;
}

static ptrdiff_t search(
    const HashTable* ht,
    uint64_t hash,
    const void* key,
    bool return_deleted
) {
    ptrdiff_t slot = hash % ht->ind_cap;
    while (
        ht->indices[slot] != IND_EMPTY
        && !(
            return_deleted
            && ht->indices[slot] == IND_DELETED
        )
    ) {
        if (
            keysmatch(
                hash,
                ht->ktype,
                key,
                ht->items + ht->indices[slot]
            )
        ) return slot;
        slot = (slot + 1) % ht->ind_cap;
    }
    return slot;
}

static void rehash(HashTable* ht)
{
    ht->ind_deleted = 0;
    util_arrset(ht->indices, ht->ind_cap, IND_EMPTY);
    for (ptrdiff_t i = 0; i < ht->item_next; i++) {
        Item* item = ht->items + i;
        if (item->usable) {
            ptrdiff_t slot = search(ht, item->hash, &item->key, false);
            ht->indices[slot] = i;
        }
    }
}


// <3> Insertion

static bool resize_indices(HashTable* ht, ptrdiff_t newcap)
{
    ptrdiff_t* new = malloc(newcap * sizeof(*new));
    if (!new) return false;
    free(ht->indices);
    ht->indices = new;
    ht->ind_cap = newcap;
    rehash(ht);
    return true;
}

static bool resize_items(HashTable* ht, ptrdiff_t newcap)
{
    Item* new = malloc(newcap * sizeof(*new));
    if (!new) return false;
    memcpy(new, ht->items, ht->item_next * sizeof(*ht->items));
    free(ht->items);
    ht->items = new;
    ht->item_cap = newcap;
    return true;
}

static void copykey(enum ht_ktype ktype, const void* key, Item* item)
{
    if (ktype == HT_INT) {
        item->key.i = *(const int64_t*)key;
    } else if (ktype == HT_STR) {
        size_t slen = util_membersize(Item, key.s) - 1;
        strncpy(item->key.s, key, slen);
        item->key.s[slen] = '\0';
    }
}

int64_t* ht_insert(HashTable* ht, const void* key, int64_t value)
{
    // test for existence
    uint64_t hash = gethash(ht->ktype, key);
    ptrdiff_t slot = search(ht, hash, key, false);
    if (ht->indices[slot] != IND_EMPTY)
        return &(ht->items[ht->indices[slot]].value);

    // resize arrays if required
    if (
        (ht->ind_usable + ht->ind_deleted + 1)
        / (double)ht->ind_cap
        > LOAD_FACTOR
        && !resize_indices(ht, ht->ind_cap * 2)
    ) return NULL;
    if (
        ht->item_next == ht->item_cap
        && !resize_items(ht, ht->item_cap * 2)
    ) return NULL;

    slot = search(ht, hash, key, true);
    ht->ind_usable++;
    ht->indices[slot] = ht->item_next++;
    Item* item = ht->items + ht->indices[slot];
    item->usable = true;
    item->hash = hash;
    item->value = value;
    copykey(ht->ktype, key, item);
    return &item->value;
}

bool ht_delete(HashTable* ht, const void* key)
{
    ptrdiff_t slot = search(ht, gethash(ht->ktype, key), key, false);
    if (ht->indices[slot] == IND_EMPTY)
        return false;
    ht->items[ht->indices[slot]].usable = false;
    ht->indices[slot] = IND_DELETED;
    ht->ind_usable--;
    ht->ind_deleted++;
    return true;
}


// <4> General

int64_t* ht_get(const HashTable* ht, const void* key)
{
    ptrdiff_t slot = search(ht, gethash(ht->ktype, key), key, false);
    return (ht->indices[slot] == IND_EMPTY)
        ? NULL
        : &ht->items[ht->indices[slot]].value;
}

enum ht_ktype ht_ktype(const HashTable* ht)
{
    return ht->ktype;
}

ptrdiff_t ht_count(const HashTable* ht)
{
    return ht->ind_usable;
}

void ht_resetiter(HashTable* ht)
{
    ht->next = 0;
}

const void* ht_next(HashTable* ht, int64_t* value)
{
    while (
        ht->next < ht->item_next
        && !(ht->items[ht->next].usable)
    ) ht->next++;
    if (ht->next == ht->item_next)
        return NULL;
    Item* item = ht->items + ht->next++;
    if (value != NULL)
        *value = item->value;
    return &item->key;
}

#define MK_AGG(name, initial, opfunc) \
int64_t ht_##name(const HashTable* ht) \
{ \
    int64_t agg = (initial); \
    for (ptrdiff_t i = 0; i < ht->item_next; i++) { \
        Item* item = ht->items + i; \
        if (!item->usable) continue; \
        agg = opfunc(agg, item->value); \
    } \
    return agg; \
}
MK_AGG(sum, 0, util_add)
MK_AGG(max, INT64_MIN, util_max)
MK_AGG(min, INT64_MAX, util_min)


// <5> Sort
// Deleted items are sorted to the end.

static enum ht_ktype st_sort_ktype;
static bool st_sort_bykey;
static int st_sort_ascending;

static int compar(const void* x, const void *y)
{
    const Item* a = x;
    const Item* b = y;
    if (!a->usable)
        return b->usable;
    if (!b->usable)
        return -a->usable;

    if (!st_sort_bykey)
        return st_sort_ascending * (
            (a->value > b->value) - (a->value < b->value)
        );

    switch (st_sort_ktype) {
        case HT_INT:
            return st_sort_ascending * (
                (a->key.i > b->key.i) - (a->key.i < b->key.i)
            );
        case HT_STR:
            return st_sort_ascending * strcmp(a->key.s, b->key.s);
    }
    return 0;
}

/* This clears deleted pointers from `pdata`. */
void ht_sort(HashTable* ht, bool bykey, bool ascending)
{
    st_sort_ktype = ht->ktype;
    st_sort_bykey = bykey;
    st_sort_ascending = ascending ? 1 : -1;
    qsort(
        ht->items,
        ht->item_next,
        sizeof(*ht->items),
        compar
    );
    rehash(ht);
    ht->item_next = ht->ind_usable;
}
