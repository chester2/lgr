#include <getopt.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "util.h"
#include "date.h"
#include "recordlist.h"
#include "program.h"

#define HELP "\
Plot monthly amount totals in the given time range.\n\
Usage: " PROG_NAME " plot [-c <cat>] [-d <desc>] [<year0> [<year1>]]\n\
       " PROG_NAME " plot [-c <cat>] [-d <desc>] -a\n\
\n\
If no time range arguments are provided, use the latest 13 months.\n\
\n\
Options:\n\
    -a          Include all transactions regardless of date.\n\
    -c <cat>    Comma-separated patterns to filter categories with.\n\
    -d <desc>   Comma-separated patterns to filter descriptions with.\n\
\n\
Positional arguments:\n\
    <year0>     First year of time range. 'yN' or integer from 1 to 9999.\n\
    <year1>     Last year of time range. 'yN' or integer from 1 to 9999. If\n\
                omitted, <year0> will be used.\n\
"

int main_plot(int argc, char** argv)
{
    // parse options
    enum usagetype {NORMAL, ALL};
    enum usagetype usagetype = NORMAL;
    const char* cat = NULL;
    const char* desc = NULL;
    optind = PROG_ARGSTART;
    for (struct option longopts[] = {{0}};;) {
        int c = getopt_long(argc, argv, ":hc:d:a", longopts, NULL);
        if (c == -1) break;
        switch (c) {
            case 'h':
                prog_pexit(HELP);
            case 'c':
                cat = optarg;
                break;
            case 'd':
                desc = optarg;
                break;
            case 'a':
                usagetype = ALL;
                break;
            case ':':
                prog_err_optnoval(optopt);
            default:
                prog_err_optunknown(optopt);
        }
    }
    prog_loadconf();

    // parse time range bounds
    argc -= optind;
    argv += optind;
    int32_t dt0, dt1;
    if (usagetype == NORMAL) {
        int y;
        if (argc == 0) {
            int32_t curmon = dt_setd(dt_today(), 1);
            dt0 = dt_shiftm(curmon, -12);
            dt1 = dt_shiftd(dt_shiftm(curmon, 1), -1);
        } else if (!prog_parsey(argv[0], &y)) {
            prog_err("invalid <year0>");
        } else {
            dt0 = dt_dt(y, 1, 1);
            if (argc < 2)
                dt1 = dt_dt(y, 12, 31);
            else if (!prog_parsey(argv[1], &y))
                prog_err("invalid <year1>");
            else if (y < dt_gety(dt0))
                prog_err("<year1> preceeds <year0>");
            else
                dt1 = dt_dt(y, 12, 31);
        }
    }

    // init/slice/filter record list
    prog_initrl();
    ptrdiff_t slicelen = (usagetype == ALL)
        ? rl_count()
        : rl_slice(dt0, dt1);
    if (cat)
        slicelen = rl_filtercat(cat, ',');
    if (desc)
        slicelen = rl_filterdesc(desc, ',');

    // handle no records
    if (slicelen == 0) {
        if (usagetype != ALL)
            prog_printdaterange(dt0, dt1);
        prog_pexit("No transactions.");
    }

    // correct date limits
    {
        int32_t dt = rl_get(rl_slicestart())->dt;
        if (usagetype == ALL || dt0 < dt)
            dt0 = dt_setd(dt, 1);
        dt = rl_get(rl_slicestop() - 1)->dt;
        if (usagetype == ALL || dt1 > dt)
            dt1 = dt_shiftd(dt_shiftm(dt_setd(dt, 1), 1), -1);
    }

    // print
    #ifdef _WIN32
    system("color");
    #endif
    void plot(int32_t dt0, int32_t dt1);
    plot(dt0, dt1);
    exit(EXIT_SUCCESS);
}


// Plotting

#define MAXBARS 60
#define BAR "\xe2\x96\x88"
#define sTOP "\xe2\x94\xac"
#define sMID "\xe2\x94\xbc"
#define sBOT "\xe2\x94\xb4"
#define sWALL "\xe2\x94\x82"
#define COLOR_IN "\033[92m"
#define COLOR_OUT "\033[91m"
#define COLOR_END "\033[0m"

/*
 *         -
 * yyyy mmm|######################### 123.02
 *         |############ 67.00
 *         -
 * yyyy mmm|
 *         |
 *         -
 * yyyy mmm|
 *         |################################ 144.11
 *         -
 * \_______/\_______________________________/
 *     A                Bar
 * 
 * In the record list slice, if either in or out have not transactions,
 * that bar will not be drawn. At least one one of in/out must have
 * transactions for plotting to happen.
 */

typedef struct {
    ptrdiff_t months;
    int64_t max;
    int64_t totpos;
    int64_t totneg; // absolute value
} PrintMeta;

typedef struct {
    int64_t pos;
    int64_t neg;    // absolute value
    char label[sizeof "yyyy mmm"];
} MonthEntry;

/* Draw A for lines that do not show "yyyy mmm". */
static void drawa(const char* vdivider)
{
    for (
        int i = 0
        ; i < (int)(util_membersize(MonthEntry, label) - 1)
        ; i++
    ) putc(' ', stdout);
    fputs(vdivider, stdout);
}

/* Draw horizontal bar and include a trailing space. */
static void drawbar(int sign, int len)
{
    fputs(sign >= 0 ? COLOR_IN : COLOR_OUT, stdout);
    for (int i = 0; i < len; i++)
        fputs(BAR, stdout);
    fputs(COLOR_END " ", stdout);
}

static void plotmonth(PrintMeta* meta, MonthEntry* entry)
{
    char buf[sizeof "92,233,720,368,547,758.08"];

    // month and vdivider
    fputs(entry->label, stdout);
    fputs(sWALL, stdout);

    if (meta->totpos > 0) {
        // positive bar
        drawbar(1, lround((double)entry->pos / meta->max * MAXBARS));
        if (entry->pos > 0) {
            buf[util_fmtcents(entry->pos, buf)] = '\0';
            fputs(buf, stdout);
        }
        putc('\n', stdout);

    // negative bar (remainder of this function)
        if (meta->totneg == 0)
            return;
        drawa(sWALL);
    }
    drawbar(-1, lround((double)entry->neg / meta->max * MAXBARS));
    if (entry->neg != 0) {
        buf[util_fmtcents(entry->neg, buf)] = '\0';
        fputs(buf, stdout);
    }
    putc('\n', stdout);
}

/* RL must already be sliced. */
void plot(int32_t dt0, int32_t dt1)
{
    // get number of months spanning ts1 to ts2
    PrintMeta meta = {
        .months = (
            12 * (dt_gety(dt1) - dt_gety(dt0))
            + (dt_getm(dt1) - dt_getm(dt0))
            + 1
        )
    };

    // get monthly totals and labels
    MonthEntry* entries = calloc(meta.months, sizeof(*entries));
    if (entries == NULL)
        prog_err_nomem();

    // populate monthentry array data data
    ptrdiff_t j = 0;
    for (
        int32_t next, cur = dt0
        ; cur <= dt1
        ; cur = next, j++
    ) {
        next = dt_shiftm(cur, 1);

        sprintf(entries[j].label, "%04d %s", dt_gety(cur), dt_mmm(dt_getm(cur)));

        rl_slice(cur, dt_shiftd(next, -1));
        for (ptrdiff_t i = rl_slicestart(); i < rl_slicestop(); i++) {
            int64_t amt = rl_get(i)->amt;
            if (amt >= 0)
                entries[j].pos += amt;
            else
                entries[j].neg -= amt;
        }
    }

    // fill rest of meta
    for (ptrdiff_t i = 0; i < meta.months; i++) {
        meta.totpos += entries[i].pos;
        meta.totneg += entries[i].neg;
        meta.max = util_max(meta.max, entries[i].pos);
        meta.max = util_max(meta.max, entries[i].neg);
    }

    // plot
    drawa(sTOP "\n");
    plotmonth(&meta, entries);
    for (ptrdiff_t i = 1; i < meta.months; i++) {
        drawa(sMID "\n");
        plotmonth(&meta, entries + i);
    }
    drawa(sBOT "\n");
}
