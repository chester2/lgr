// <1> General
// <2> Printing
#include <stdbool.h>
#include <stdlib.h>

#include "util.h"
#include "date.h"
#include "hashtable.h"
#include "record.h"
#include "recordlist.h"
#include "recordtree.h"

typedef struct node {
    union {
        struct node* nodes;
        const Record* records;
    } ch;
    ptrdiff_t chlen;
    int id;
} Node;

/* The array of nodes. */
static Node* st_nodes;

/* Traversal entry point. */
static Node st_root;

/* Number of day nodes. */
static ptrdiff_t st_uniquedates;

/*
 * Node hierarchy is root -> year -> month -> day. All nodes except root
 * are stored in a single array with the following layout:
 * 
 * +-------------+
 * | day nodes   |
 * |             |
 * +-------------+
 * | month nodes |
 * |             |
 * +-------------+
 * | year nodes  |
 * |             |
 * +-------------+
 * 
 * Each node's children are stored contiguously, ordered by ID. Day nodes'
 * children are record pointers stored in a record list (recall those
 * pointers are sorted by timestamp). A record tree does not own any record
 * objects, and the record list from which a record tree was built must
 * remain allocated in order to use the record tree.
 */


// <1> General

static Node* initnode(Node* node, int id, ptrdiff_t chlen, Node* children)
{
    node->id = id;
    node->chlen = chlen;
    node->ch.nodes = children;
    return node;
}

bool rt_init(void)
{
    bool return_status = false;

    // count unique years, year-months, and dates
    // years keys are year numbers
    // months keys are (y*100 + m)
    // days keys are dates from "date.h"
    HashTable* ycounter = ht_new(HT_INT);
    HashTable* mcounter = ht_new(HT_INT);
    HashTable* dcounter = ht_new(HT_INT);
    if (ycounter == NULL || mcounter == NULL || dcounter == NULL)
        goto cleanup;

    for (ptrdiff_t i = rl_slicestart(); i < rl_slicestop();) {
        int32_t dt = rl_get(i)->dt;
        while (
            i < rl_slicestop()
            && dt == rl_get(i)->dt
        ) i++;

        int64_t key;
        if (
            NULL == ht_insert(dcounter, (key=dt, &key), 0)
            || NULL == ht_insert(mcounter, (key=dt/100, &key), 0)
            || NULL == ht_insert(ycounter, (key=dt/10000, &key), 0)
        ) goto cleanup;
    }

    // create the node array
    Node* arr = malloc(
        sizeof(*arr) * (
            ht_count(ycounter) + ht_count(mcounter) + ht_count(dcounter)
        )
    );
    if (arr == NULL) {
        goto cleanup;
    } else {
        ht_sort(dcounter, true, true);
        Node* dnode = arr;
        Node* mnode = dnode + ht_count(dcounter) - 1;
        Node* ynode = mnode + ht_count(mcounter);

        int prevy = -1, prevm = -1;
        const void* key;
        ht_foreach(dcounter, key, NULL) {
            int32_t dt = *(const int64_t*)key;
            int y = dt_gety(dt);
            int m = dt_getm(dt);
            int d = dt_getd(dt);
            initnode(dnode, d, 0, NULL);
            if (y == prevy && m == prevm) {
                mnode->chlen++;
            } else {
                initnode(++mnode, m, 1, dnode);
                if (y == prevy) 
                    ynode->chlen++;
                else
                    initnode(++ynode, y, 1, mnode);
            }
            dnode++;
            prevy = y;
            prevm = m;
        }
    }

    // set attach records to day nodes
    for (ptrdiff_t k = 0, i = rl_slicestart(); i < rl_slicestop(); k++) {
        ptrdiff_t j = i + 1;
        int32_t dt = rl_get(i)->dt;
        while (
            j < rl_slicestop()
            && dt == rl_get(j)->dt
        ) j++;
        arr[k].chlen = j - i;
        arr[k].ch.records = rl_get(i);
        i = j;
    }

    // initialize
    st_uniquedates = ht_count(dcounter);
    st_nodes = arr;
    initnode(
        &st_root,
        0,
        ht_count(ycounter),
        arr + ht_count(dcounter) + ht_count(mcounter)
    );

    return_status = true;
cleanup:
    ht_free(ycounter);
    ht_free(mcounter);
    ht_free(dcounter);
    return return_status;
}

void rt_deinit(void)
{
    if (st_nodes) {
        free(st_nodes);
        st_nodes = NULL;
        st_root = (Node){0};
        st_uniquedates = 0;
    }
}


// <2> Printing

#define VFMT_MINDD 2
#define DD "\xe2\x94\x80"
#define sT "\xe2\x94\x9c" DD DD " "
#define sL "\xe2\x94\x94" DD DD " "
#define sI "\xe2\x94\x82   "
#define s_ "    "

/*
 * Print format is as follows:
 *  yyyy
 *  `-- mmm
 *    `-- d
 *        `-- {i} {DD} {amt} {cat}: {desc}
 *            |____________|
 *                 VFMT
 * 
 * VFMT:
 *  i
 *      Record's index in its containing date node.
 *  DD
 *      A sequence of two or more U+2500 characters to pad VFMT such that
 *      every record's VFMT has the same character length.
 *  amt
 *      Record's amt formatted to two decimal places with thousands separator.
 *      Parentheses indicate negative quantities. Positive quantities must
 *      have a trailing space to match the closing parentheses of negative
 *      quantities. Examples:
 *           "1,018,821,00 "
 *          "(1,018,821,00)"
 */

typedef struct {
    FILE* stream;
    int vfmtclen;       // VFMT character length
    const char* rec_yprefix;  // record line year column prefix
    const char* rec_mprefix;  // record line month column prefix
} PrintState;

/* Write a record's VFMT to STREAM. */
static void print_vfmt(FILE* stream, int dind, int64_t amt, int vfmtlen)
{
    char amtbuf[sizeof "(92,233,720,368,547,758.08)"];
    int amtlen = util_fmtcents(amt, amtbuf);
    if (amt >= 0) {
        amtbuf[amtlen] = ' ';
    } else {
        amtbuf[0] = '(';
        amtbuf[amtlen] = ')';
    }
    amtbuf[++amtlen] = '\0';    // add 1 to amtlen for the trailing ' ' or ')'
    int dindlen = fprintf(stream, "%d", dind);
    putc(' ', stream);
    for (
        int i = dindlen + 2 + amtlen
        ; i < vfmtlen
        ; i++
    ) fputs(DD, stream);
    putc(' ', stream);
    fputs(amtbuf, stream);
}

static ptrdiff_t print_rec(
    const Record* rec,
    const PrintState* pstate,
    bool isfinal,
    ptrdiff_t dind
) {
    FILE* stream = pstate->stream;
    fputs(pstate->rec_yprefix, stream);
    fputs(pstate->rec_mprefix, stream);
    fputs(isfinal ? sL : sT, stream);
    print_vfmt(stream, dind, rec->amt, pstate->vfmtclen);
    putc(' ', stream);
    fputs(rec->cat, stream);
    if (*rec->desc) {
        fputs(": ", stream);
        fputs(rec->desc, stream);
    }
    putc('\n', stream);
    return 1;
}

static long print_day(const Node* day, PrintState* pstate, bool isfinal)
{
    const char* suffix;
    {
        int ones = day->id % 10;
        int tens;
        if (ones > 3 || (tens = day->id / 10 % 10) == 1)
            suffix = "th";
        else if (ones == 1)
            suffix = "st";
        else if (ones == 2)
            suffix = "nd";
        else
            suffix = "rd";
    }

    fputs(pstate->rec_yprefix, pstate->stream);
    fputs(isfinal ? sL : sT, pstate->stream);
    fprintf(pstate->stream, "%d", day->id);
    fputs(suffix, pstate->stream);
    putc('\n', pstate->stream);
    ptrdiff_t lines = 1;

    pstate->rec_mprefix = isfinal ? s_ : sI;
    ptrdiff_t i = 0;
    for (; i < day->chlen - 1; i++)
        lines += print_rec(day->ch.records + i, pstate, false, i + RT_UI_FIRST_DIND);
    lines += print_rec(day->ch.records + i, pstate, true, i + RT_UI_FIRST_DIND);

    return lines;
}

static ptrdiff_t print_month(const Node* month, PrintState* pstate, bool isfinal)
{
    fputs(isfinal ? sL : sT, pstate->stream);
    fputs(dt_mmm(month->id), pstate->stream);
    putc('\n', pstate->stream);
    ptrdiff_t lines = 1;

    pstate->rec_yprefix = isfinal ? s_ : sI;
    ptrdiff_t i = 0;
    for (; i < month->chlen - 1; i++)
        lines += print_day(month->ch.nodes + i, pstate, false);
    lines += print_day(month->ch.nodes + i, pstate, true);

    return lines;
}

static ptrdiff_t print_year(const Node* year, PrintState* pstate)
{
    fprintf(pstate->stream, "%04d\n", year->id);
    ptrdiff_t lines = 1;

    ptrdiff_t i = 0;
    for (; i < year->chlen - 1; i++)
        lines += print_month(year->ch.nodes + i, pstate, false);
    lines += print_month(year->ch.nodes + i, pstate, true);

    return lines;
}

ptrdiff_t rt_print(FILE* stream)
{

    // set encoding
    PrintState pstate = {.stream = stream};

    // calculate vfmtclen
    {
        int maxdindlen;
        {
            int64_t max = 0;
            for (ptrdiff_t i = 0; i < st_uniquedates; i++)
                max = util_max(max, st_nodes[i].chlen - 1);
            maxdindlen = util_digits(max + RT_UI_FIRST_DIND, false);
        }

        int maxamtlen;
        {
            int64_t max = 0, min = 0;
            for (ptrdiff_t i = 0; i < st_uniquedates; i++) {
                Node* dnode = st_nodes + i;
                for (ptrdiff_t j = 0; j < dnode->chlen; j++) {
                    int64_t amt = dnode->ch.records[j].amt;
                    max = util_max(max, amt);
                    min = util_min(min, amt);
                }
            }
            maxamtlen = 1 + util_max(
                util_fmtcentslen(max), util_fmtcentslen(min)
            );
        }

        // Max character length occurs with madindlen and maxamtlen in the same
        // string. In such a situation, there are exactly VFMT_MINDD DD
        // characters.
        pstate.vfmtclen = maxdindlen + maxamtlen + VFMT_MINDD + 2;
    }

    ptrdiff_t lines = 0;
    for (ptrdiff_t i = 0; i < st_root.chlen; i++)
        lines += print_year(st_root.ch.nodes + i, &pstate);
    return lines;
}
