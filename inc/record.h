#ifndef LGR_RECORD_H
#define LGR_RECORD_H

#include <stdint.h>

#include "date.h"

#define REC_DELIM "\t"

/* Amount limits. */
#define REC_AMT_MAX (100LL*1000*1000*1000*1000)
#define REC_AMT_MIN (-REC_AMT_MAX)

/* String member sizes. */
#define REC_CATLEN 23
#define REC_DESCLEN ( \
    128 \
    - (int)sizeof(int64_t) \
    - (int)sizeof(int32_t) \
    - REC_CATLEN - 1 \
    - 1 \
)

/* Large enough to store serialization of any record. */
#define REC_STRLEN ( \
    DT_ISOLEN + 1 \
    + (int)sizeof("-9223372036854775808") + 1 \
    + REC_CATLEN + 1 \
    + REC_DESCLEN \
)

/*
 * Represents a single transaction.
 * 
 * The serialization format is as follows:
 *      "{yyyy-mm-dd}\t{amount}\t{category}\t{description}"
 * 
 * A record is valid if:
 *      Amount is nonzero and between REC_AMT_MAX and REC_AMT_MIN inclusive
 *      Date is valid
 *      Category is at least one char long and does not contain TABs
 */
typedef struct {
    int64_t amt;                // transaction amount in cents
    int32_t dt;                 // date of transaction timestamp
    char cat[REC_CATLEN + 1];   // category
    char desc[REC_DESCLEN + 1]; // optional description
} Record;

/* Initialize a record, lowercasing all of CAT's characters. Return REC. If
any component is invalid, return NULL and leave REC unmodified. */
Record* rec_init(
    Record* rec, int32_t dt, int64_t amt, const char* cat, const char* desc
);

/* Deserialize string to record. Return REC on success and NULL on failure. */
Record* rec_fromstr(Record* rec, const char* s);

/* Serialize a valid record to a statically allocated string. */
char* rec_tostr(const Record* rec);

#endif
