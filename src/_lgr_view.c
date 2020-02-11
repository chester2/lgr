#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>

#include "date.h"
#include "recordlist.h"
#include "recordtree.h"
#include "program.h"

#define CMD PROG_NAME " view"

#define HELP "\
View transactions in a given time range.\n\
Usage: " CMD " [-c <cat>] [-d <desc>] [<month>] [<year>]\n\
       " CMD " [-c <cat>] [-d <desc>] -a\n\
       " CMD " [-c <cat>] [-d <desc>] -s <date0> [<date1>]\n\
\n\
If no time range arguments are provided, use the current month.\n\
\n\
Options:\n\
    -a          Include all transactions regardless of date.\n\
    -s          Use specific dates to specify time range.\n\
    -c <cat>    Comma-separated patterns to filter categories with.\n\
    -d <desc>   Comma-separated patterns to filter descriptions with.\n\
\n\
Positional arguments:\n\
    <month>     'mN' or integer from 1 to 12.\n\
    <year>      'yN' or integer from 13 to 9999.\n\
    <date0>     First date of time range. 'dN' or 'yyyy-mm-dd'.\n\
    <date1>     Last date of time range. 'dN' or 'yyyy-mm-dd'. If omitted,\n\
                <date0> will be used.\n\
\n\
Examples:\n\
    Include current month transactions:\n\
        " CMD "\n\
        " CMD " m\n\
    Include January of current year transactions:\n\
        " CMD " 1\n\
    Include this month of last year transactions:\n\
        " CMD " m y-1\n\
    Include next month of last year transactions:\n\
        " CMD " m+1 y-1\n\
    Include current year transactions:\n\
        " CMD " y\n\
    Include year 13 transactions:\n\
        " CMD " 13\n\
    Include all transactions with categories containing 'work' or 'dining' and\n\
    descriptions containing 'a':\n\
        " CMD " -a -c work,dining -d a\n\
    Include today's transactions:\n\
        " CMD " d\n\
    Include transactions spanning Jan 3, 2000 to today:\n\
        " CMD " 2000-01-03 d\n\
"

int main_view(int argc, char** argv)
{
    // parse options
    enum usagetype {NORMAL, ALL, SPECIFIC};
    enum usagetype usagetype = NORMAL;
    const char* cat = NULL;
    const char* desc = NULL;
    optind = PROG_ARGSTART;
    for (struct option longopts[] = {{0}};;) {
        int c = getopt_long(argc, argv, ":hc:d:as", longopts, NULL);
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
            case 's':
                usagetype = SPECIFIC;
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
    if (usagetype == SPECIFIC) {
        if (argc == 0)
            prog_err("<date0> missing");
        if (!prog_parsedt(argv[0], &dt0))
            prog_err("invalid <date1>");
        dt1 = dt0;
        if (argc >= 2 && !prog_parsedt(argv[1], &dt1))
            prog_err("invalid <date2>");
        if (dt1 < dt0)
            prog_err("<date1> preceeds <date0>");
    } else if (usagetype == NORMAL) {
        int m, y;
        if (argc == 0) {
            // neither month nor year given
            dt0 = dt_setd(dt_today(), 1);
            dt1 = dt_shiftd(dt_shiftm(dt0, 1), -1);
        } else if (!prog_parsem(argv[0], &m)) {
            // month not given
            if (!prog_parsey(argv[0], &y))
                prog_err("invalid <month> or <year>");
            dt0 = dt_dt(y, 1, 1);
            dt1 = dt_dt(y, 12, 31);
        } else {
            // month given
            if (argc == 1)
                y = dt_gety(dt_today());
            else if (!prog_parsey(argv[1], &y))
                prog_err("invalid <year>");
            // m might not be between 1-12 so we need to wrap it back into that range
            dt0 = dt_shiftm(dt_dt(y, 1, 1), m - 1);
            if (!dt_isdt(dt0))
                prog_err("month %d and year %d combination is invalid", m, y);
            dt1 = dt_shiftd(dt_shiftm(dt0, 1), -1);
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

    // print
    if (slicelen == 0) {
        if (usagetype != ALL)
            prog_printdaterange(dt0, dt1);
        prog_pexit("No transactions.");
    } else {
        if (!rt_init())
            prog_err_nomem();
        rt_print(stdout);
    }
    exit(EXIT_SUCCESS);
}
