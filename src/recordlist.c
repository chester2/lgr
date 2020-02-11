// <1> Getters
// <2> IO
// <3> General
// <4> Slicing

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"
#include "record.h"
#include "recordlist.h"

/* The array of records. */
static Record* st_records;

/* Number of records. */
static ptrdiff_t st_count;

/* The active slice is the range of indices I such that `slicestart` <= I <
`slicestop`. If the active slice is empty, `slicestart` and `slicestop` are
equal. */
static ptrdiff_t st_slicestart;
static ptrdiff_t st_slicestop;


// <1> Getters

const Record* rl_get(ptrdiff_t index)
{
    return (index < 0 || index >= st_count)
        ? NULL
        : st_records + index;
}

ptrdiff_t rl_count(void) {return st_count;}
ptrdiff_t rl_slicestart(void) {return st_slicestart;}
ptrdiff_t rl_slicestop(void) {return st_slicestop;}
ptrdiff_t rl_slicecount(void) {return st_slicestop - st_slicestart;}


// <2> IO

ptrdiff_t rl_init(FILE* f)
{
    // count lines and check line widths
    // too many lines means the first MAXLEN or more lines are nonempty
    ptrdiff_t lines = 0;
    if (f) {
        ptrdiff_t line_index = 0;
        int cur_linepos = 0;
        fseek(f, 0, SEEK_SET);
        for (int c; EOF != (c = getc(f));) {
            if (c != '\n') {
                if (REC_STRLEN == cur_linepos++)
                    return line_index + 1;
            } else {
                cur_linepos = 0;
                line_index++;
                if (line_index > RL_MAXCOUNT) return PTRDIFF_MAX;
            }
        }
        if (line_index < RL_MAXCOUNT)
            lines = line_index + !!cur_linepos;
        else if (cur_linepos == 0)
            lines = line_index;
        else
            return PTRDIFF_MAX;
    }

    Record* arr = malloc((lines + 1) * sizeof(*arr));
    if (arr == NULL)
        return -1;

    // deserialize lines
    if (f) {
        char buf[REC_STRLEN + 1];
        fseek(f, 0, SEEK_SET);
        for (ptrdiff_t i = 0; i < lines; i++) {
            size_t slen = strlen(fgets(buf, sizeof buf, f));
            if (buf[slen-1] == '\n')
                buf[slen-1] = 0;
            if (NULL == rec_fromstr(arr + i, buf)) {
                free(arr);
                return i + 1;
            };
        }
    }

    st_records = arr;
    st_count = lines;
    st_slicestart = 0;
    st_slicestop = st_count;
    return 0;
}

void rl_write(FILE* f)
{
    for (int i = 0; i < st_count; i++) {
        fputs(rec_tostr(st_records + i), f);
        putc('\n', f);
    }
    fflush(f);
}

void rl_deinit(void)
{
    if (st_records) {
        free(st_records);
        st_records = NULL;
        st_count = 0;
        st_slicestart = 0;
        st_slicestart = 0;
    }
}


// <3> General

/* Return the rightmost record insertion point for the record list to
remain sorted. */
static ptrdiff_t rl_bsr(int32_t dt)
{
    ptrdiff_t l = 0, r = st_count;
    while (l < r) {
        ptrdiff_t m = (l + r) / 2;
        if (dt >= st_records[m].dt)
            l = m + 1;
        else
            r = m;
    }
    return l;
}

/* Return the leftmost record insertion point for the record list to remain
sorted. */
static ptrdiff_t rl_bsl(int32_t dt)
{
    return rl_bsr(dt - 1);
}

const Record* rl_insert(const Record* rec)
{
    ptrdiff_t index = rl_bsr(rec->dt);
    for (ptrdiff_t i = st_count; i > index; i--)
        st_records[i] = st_records[i-1];
    st_records[index] = *rec;
    st_count++;
    st_slicestart += (index <= st_slicestart);
    st_slicestop += (index < st_slicestop);
    return st_records + index;
}

bool rl_delete(ptrdiff_t index)
{
    if (index < 0 || index >= st_count)
        return false;
    st_count--;
    for (ptrdiff_t i = index; i < st_count; i++)
        st_records[i] = st_records[i+1];
    st_slicestart -= (index < st_slicestart);
    st_slicestop -= (index < st_slicestop);
    return true;
}


// <4> Slicing

ptrdiff_t rl_resetslice(void)
{
    st_slicestart = 0;
    st_slicestop = st_count;
    return st_count;
}

ptrdiff_t rl_slice(int32_t dt0, int32_t dt1)
{
    st_slicestart = rl_bsl(dt0);
    st_slicestop = rl_bsr(dt1);
    return rl_slicecount();
}

/* Count the number of occurrences of C in S. If CPOSITIONS is not null, it
is an array of large enough size used to store indices of C. */
static int countchars(const char* s, int c, int* cpositions)
{
    int count = 0;
    for (int i = 0; s[i] != '\0'; i++)
        if (s[i] == c) {
            if (cpositions != NULL)
                cpositions[count] = i;
            count++;
        }
    return count;
}

/* Copy LEN chars from SRC to DEST. Set DEST's (len+1)th element to the
null byte. */
static char* lcasecpy(const char* src, char* dest, int len)
{
    for (int i = 0; i < len; i++)
        dest[i] = tolower(src[i]);
    dest[len] = '\0';
    return dest;
}

#define MK_FILTER(member) \
ptrdiff_t rl_filter##member(const char* patterns, int delim) \
{ \
    /* Make a copy of PATTERNS lowercased, with all DELIM characters
    changed to the null byte. Create an array tracking the location of each
    DELIM character. For each record member, check if at least one of the
    patterns in PATTERN's copy is a substring of that member. */ \
\
    /* -1 for malloc failure. */ \
    ptrdiff_t retval = -1;  \
\
    int* delim_positions = NULL; \
    char* patterns_lcased = NULL; \
\
    /* Get delim positions. */ \
    int delim_count = countchars(patterns, delim, NULL); \
    delim_positions = malloc(delim_count * sizeof(*delim_positions)); \
    if (delim_positions == NULL) \
        goto cleanup; \
    countchars(patterns, delim, delim_positions); \
\
    /* Lcase-copy patterns. */ \
    int patlen = strlen(patterns); \
    patterns_lcased = malloc(patlen + 1); \
    if (patterns_lcased == NULL) \
        goto cleanup; \
    lcasecpy(patterns, patterns_lcased, patlen); \
    for (int i = 0; i < delim_count; i++) \
        patterns_lcased[delim_positions[i]] = '\0'; \
\
    for (ptrdiff_t i = st_slicestop - 1; i >= st_slicestart; i--) { \
        /* Lcase-copy member. */ \
        char curmember[util_membersize(Record, member)]; \
        lcasecpy(st_records[i].member, curmember, util_membersize(Record, member) - 1); \
\
        /* Substring compare. */ \
        if (strstr(curmember, patterns_lcased)) \
            continue; \
        for (int j = 0; j < delim_count; j++) \
            if (strstr(curmember, patterns_lcased + delim_positions[j] + 1)) \
                goto matches; \
\
        rl_delete(i); \
    matches:; \
    } \
\
    retval = rl_slicecount(); \
cleanup: \
    free(delim_positions); \
    free(patterns_lcased); \
    return retval; \
}
MK_FILTER(cat)
MK_FILTER(desc)
