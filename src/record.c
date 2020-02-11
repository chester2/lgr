#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "util.h"
#include "date.h"
#include "record.h"

Record* rec_init(Record* rec, int32_t dt, int64_t amt, const char* cat, const char* desc)
{
    if (
        !amt
        || amt > REC_AMT_MAX
        || amt < REC_AMT_MIN
        || !*cat
        || strchr(cat, *REC_DELIM)
        || !dt_isdt(dt)
    ) return NULL;

    rec->dt = dt;
    rec->amt = amt;

    size_t catlen = util_min(strlen(cat), REC_CATLEN);
    for (size_t i = 0; i < catlen; i++)
        rec->cat[i] = tolower(cat[i]);
    rec->cat[catlen] = '\0';

    size_t desclen = util_min(strlen(desc), REC_DESCLEN);
    memcpy(rec->desc, desc, desclen);
    rec->desc[desclen] = '\0';

    return rec;
}

Record* rec_fromstr(Record* rec, const char* s)
{
    enum {DT, AMT, CAT, DESC, ATTRCOUNT};

    // copy s to scpy, replacing DELIM with NUL
    // ensure DELIM appears at least 3 times
    char scpy[REC_STRLEN + 1];
    char* attrptrs[ATTRCOUNT];
    {
        const char* src = s;
        char* dest = scpy;
        attrptrs[0] = dest;
        int delims = 0;
        for (; *src; src++, dest++) {
            if (*src != *REC_DELIM || delims >= ATTRCOUNT - 1) {
                *dest = *src;
            } else {
                *dest = '\0';
                attrptrs[++delims] = dest + 1;
            }
        }
        if (delims < ATTRCOUNT - 1) return NULL;
        *dest = '\0';
    }

    int64_t amt;
    return (
        (amt = util_stoi(attrptrs[AMT], NULL))
        && dt_isiso(attrptrs[DT])
        && rec_init(
            rec,
            dt_fromiso(attrptrs[DT]),
            amt,
            attrptrs[CAT],
            attrptrs[DESC]
        )
    ) ? rec : NULL;
}

char* rec_tostr(const Record* rec)
{
    static char buf[REC_STRLEN + 1];
    sprintf(
        buf,
        "%s" REC_DELIM "%" PRId64 REC_DELIM "%s" REC_DELIM "%s",
        dt_toiso(rec->dt),
        rec->amt,
        rec->cat,
        rec->desc
    );
    return buf;
}
