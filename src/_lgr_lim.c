#include <getopt.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "hashtable.h"
#include "recordlist.h"
#include "program.h"

#define HELP "\
View, set, or remove contribution limits.\n\
Usage: " PROG_NAME " lim [-r | -t]\n\
Usage: " PROG_NAME " lim <year> <limit>\n\
\n\
Options:\n\
    -r          Show current year's remaining RRSP limit. This is the default.\n\
    -t          Show current year's remaining TFSA limit.\n\
\n\
Positional arguments:\n\
    <year>      'yN' or integer from 0 to 9999.\n\
    <limit>     <year>'s contribution limit increase. Provide 0 to remove that\n\
                year's limit.\n\
"

void main_lim(int argc, char** argv)
{
    enum usagetype {SET, RRSP, TFSA};
    enum usagetype usagetype = SET;
    optind = PROG_ARGSTART;
    for (struct option longopts[] = {{0}};;) {
        int c = getopt_long(argc, argv, ":hrt", longopts, NULL);
        if (c == -1) break;
        switch (c) {
            case 'h':
                prog_pexit(HELP);
            case 'r':
                usagetype = RRSP;
                break;
            case 't':
                usagetype = TFSA;
                break;
            case ':':
                prog_err_optnoval(optopt);
            default:
                prog_err_optunknown(optopt);
        }
    }
    prog_loadconf();

    int year;
    int64_t limit;
    if (usagetype == SET) {
        argc -= optind;
        argv += optind;
        if (argc >= 2) {
            if (!prog_parsey(argv[0], &year))
                prog_err("invalid <year>");
            if (!prog_parsecents(argv[1], &limit))
                prog_err("<limit> invalid or out of range");
            if (limit < 0)
                prog_err("<limit> must be non-negative");
        } else {
            switch (prog_getconf(PROG_CONF_LIM_TYPE)) {
                case 't':
                    usagetype = TFSA;
                    break;
                default:
                    usagetype = RRSP;
                    break;
            }
        }
    }

    HashTable* read_lim(void);
    void set_lim(HashTable* ht, int64_t year, int64_t limit);
    void print_tfsa(HashTable* ht);
    void print_rrsp(HashTable* ht);

    HashTable* ht = read_lim();
    if (usagetype != SET) {
        prog_initrl();
        ht_sort(ht, true, true);
    }
    switch (usagetype) {
        case SET:
            set_lim(ht, year, limit);
            break;
        case RRSP:
            print_rrsp(ht);
            break;
        case TFSA:
            print_tfsa(ht);
            break;
    }
    exit(EXIT_SUCCESS);
}


// <1> Set

/* Read yearly limits into a hash table. Exit program on error. */
HashTable* read_lim(void)
{
    HashTable* ht = ht_new(HT_INT);
    if (ht == NULL)
        prog_err_nomem();
    if (!util_fexists(PROG_LIMFN))
        return ht;
    FILE* f = fopen(PROG_LIMFN, "r");
    if (f == NULL)
        prog_err_read(PROG_LIMFN);

    for (ptrdiff_t lineno = 1;; lineno++) {
        char buf[sizeof "yyyy=-9223372036854775808\n"];
        char *skey, *svalue;
        {
            int status = util_readiniline(f, sizeof buf, buf, &skey, &svalue);
            if (status == -1)
                break;
            else if (status == 1)
                prog_err("%s:%td: line too long", PROG_LIMFN, lineno);
            else if (status == 2)
                prog_err("%s:%td: invalid line", PROG_LIMFN, lineno);
        }

        long long year = util_stoi(skey, NULL);
        long long value = util_stoi(svalue, NULL);
        if (
            year == 0
            || !dt_isy(year)
            || value <= 0
            || value > INT64_MAX
        ) prog_err("%s:%td: invalid line", PROG_LIMFN, lineno);

        int64_t key = year;
        ht_insert(ht, &key, value);
    }

    fclose(f);
    return ht;
}

/* Write hash table containing yearly limits to disk. Exit program on error. */
void set_lim(HashTable* ht, int64_t year, int64_t limit)
{
    char success_msg[sizeof "yyyy -> 92,233,720,368,547,758.07"];

    if (limit == 0) {
        if (!ht_delete(ht, &year))
            prog_err("%d's limit not yet set", (int)year);
        sprintf(success_msg, "Removed year %d's limit.", (int)year);
    } else {
        int64_t* value = ht_insert(ht, &year, 0);
        if (value == NULL)
            prog_err_nomem();
        *value = limit;
        ht_sort(ht, true, true);
        char* limstart = success_msg + sprintf(success_msg, "%d -> ", (int)year);
        limstart[util_fmtcents(limit, limstart)] = '\0';
    }

    FILE* f = fopen(PROG_LIMFN, "w");
    if (f == NULL)
        prog_err_write(PROG_LIMFN);

    const void* key;
    int64_t value;
    ht_foreach(ht, key, &value) {
        fprintf(
            f,
            "%d=%" PRId64 "\n",
            (int)*(const int64_t*)key,
            value
        );
    }
    fclose(f);

    puts(success_msg);
}


// <2> Print

/* Assume hash table is sorted. */
static int64_t to_thisyear_lim(HashTable* ht)
{
    int thisyear = dt_gety(dt_today());
    int64_t acc = 0;
    const void* key;
    int64_t value;
    ht_foreach(ht, key, &value) {
        if (*(const int64_t*)key > thisyear)
            break;
        acc += value;
    }
    return acc;
}

/* Assume record list is initialized. */
static int64_t to_thisyear_in(void)
{
    rl_slice(
        dt_dt(1, 1, 1),
        dt_dt(dt_gety(dt_today()), 12, 31)
    );
    int64_t acc = 0;
    for (ptrdiff_t i = rl_slicestart(); i < rl_slicestop(); i++) {
        int64_t amt = rl_get(i)->amt;
        if (amt > 0)
            acc += amt;
    }
    return acc;
}

/* Assume record list is initialized. Return absolute value. */
static int64_t to_lastyear_out(void)
{
    rl_slice(
        dt_dt(1, 1, 1),
        dt_dt(dt_gety(dt_today()) - 1, 12, 31)
    );
    int64_t acc = 0;
    for (ptrdiff_t i = rl_slicestart(); i < rl_slicestop(); i++) {
        int64_t amt = rl_get(i)->amt;
        if (amt < 0)
            acc -= amt;
    }
    return acc;
}

/* Assume hash table is sorted. */
static void print_lims(HashTable* ht)
{
    if (ht_count(ht) == 0) return;

    int max_centslen = util_fmtcentslen(ht_max(ht));
    char buf[sizeof "92,233,720,368,547,758.07"];

    const void* key;
    int64_t value;
    ht_foreach(ht, key, &value) {
        buf[util_fmtcents(value, buf)] = '\0';
        printf(
            "%4d: %*s\n",
            (int)*(const int64_t*)key,
            max_centslen,
            buf
        );
    }

    for (
        int i = 0
        ; i + 1 < (int)(sizeof "yyyy: ") + max_centslen
        ; i++
    ) putc('-', stdout);
    putc('\n', stdout);
}

static void print_rem(int64_t rem)
{
    char buf[sizeof "-92,233,720,368,547,758.07"];
    buf[util_fmtcents(rem, buf)] = '\0';
    printf("Current year remaining: %s\n", buf);
}

/* Assume hash table is sorted. Assume record list has been initialized. */
void print_rrsp(HashTable* ht)
{
    print_lims(ht);
    print_rem(to_thisyear_lim(ht) - to_thisyear_in());
}

/* Assume hash table is sorted. Assume record list has been initialized. */
void print_tfsa(HashTable* ht)
{
    print_lims(ht);
    print_rem(
        to_thisyear_lim(ht)
        - to_thisyear_in()
        + to_lastyear_out()
    );
}
