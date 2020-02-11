/*
 * Hash table mapping integers/strings to integers. Maintains insertion
 * order. Can be sorted by keys or values.
 */

#ifndef LGR_HASHTABLE_H
#define LGR_HASHTABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Valid key types. */
enum ht_ktype {HT_INT, HT_STR};

typedef struct hashtable HashTable;

/* Return NULL if insufficient memory. */
HashTable* ht_new(enum ht_ktype ktype);

void ht_free(HashTable* ht);

/* Insert key value pair. Return a pointer to the value stored in the hash
table. If KEY already exists, its value will not be modified. Return NULL
if there is insufficient memory. */
int64_t* ht_insert(HashTable* ht, const void* key, int64_t value);

/* Delete a key. Return false if KEY does not exist. */
bool ht_delete(HashTable* ht, const void* key);

/* Return a pointer to KEY's value, or NULL if KEY does not exist. Use this
function to test for existence of keys. */
int64_t* ht_get(const HashTable* ht, const void* key);

/* Return the hash table's key type. */
enum ht_ktype ht_ktype(const HashTable* ht);

/* Number of key-value pairs. */
ptrdiff_t ht_count(const HashTable* ht);

/* Prepare hash table for iteration. */
void ht_resetiter(HashTable* ht);

/* Iterate the hashtable, returning the next item's key. If VALUE is not
NULL, use it to store the next item's value. Return NULL once all items
have been exhausted. */
const void* ht_next(HashTable* ht, int64_t* value);

/* Convenience macro for setting up a for loop to iterate through hash
table. KEY must be an lvalue of type (const void*). VALUE must have type
(int64_t*). */
#define ht_foreach(ht, key, value) for ( \
    ht_resetiter((ht)) \
    ; (key = ht_next((ht), (value))) \
    ; \
)

/* Return the sum of all values. Does not check overflow. Return 0 if hash
table is empty. */
int64_t ht_sum(const HashTable* ht);

/* Return max value. Return INT64_MIN if hashtable is empty. */
int64_t ht_max(const HashTable* ht);

/* Return min value. Return INT64_MAX if hashtable is empty. */
int64_t ht_min(const HashTable* ht);

/* Sort items for iterating over. If BYKEY is false, will sort by value.
This function is not re-entrant. */
void ht_sort(HashTable* ht, bool bykey, bool ascending);

#endif
