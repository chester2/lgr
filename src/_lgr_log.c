#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "date.h"
#include "record.h"
#include "recordlist.h"
#include "recordtree.h"
#include "program.h"

#define HELP "\
Log a transaction.\n\
Usage: " PROG_NAME " log [-i | -o] [-d <desc>] <cat> <amt> [<date>]\n\
\n\
Options:\n\
    -i          <amt> is inflow. This is the default.\n\
    -o          <amt> is outflow.\n\
    -d <desc>   Transaction description. ASCII characters only.\n\
\n\
Positional arguments:\n\
    <cat>       Transaction category. ASCII characters only.\n\
    <amt>       Transaction amount.\n\
    <date>      Transaction date. Specify a day offset N from today by\n\
                inputting 'dN', or specify an exact date with 'yyyy-mm-dd'.\n\
"

void main_log(int argc, char** argv)
{
    // parse options; get amt sign and desc if given
    char* desc = "";
    int sign = 0;
    optind = PROG_ARGSTART;
    for (struct option longopts[] = {{0}};;) {
        int c = getopt_long(argc, argv, ":hd:io", longopts, NULL);
        if (c == -1) break;
        switch (c) {
            case 'h':
                prog_pexit(HELP);
            case 'i':
                sign = 1;
                break;
            case 'o':
                sign = -1;
                break;
            case 'd':
                desc = optarg;
                if (strchr(desc, '\n'))
                    prog_err("<desc> must not contain newline characters");
                break;
            case ':':
                prog_err_optnoval(optopt);
            default:
                prog_err_optunknown(optopt);
        }
    }
    prog_loadconf();
    if (sign == 0)
        sign = (prog_getconf(PROG_CONF_LOG_SIGN) >= 0) ? 1 : -1;

    // parse other record components, then initialize record
    argc -= optind;
    argv += optind;
    if (argc < 2)
        prog_err_missingargs();
    Record rec;
    {
        // parse cat
        const char* cat = argv[0];
        if (strchr(cat, *REC_DELIM) || strchr(cat, '\n'))
            prog_err("<cat> must not contains tab or newline characters");

        // parse amt
        int64_t amt;
        if (!prog_parsecents(argv[1], &amt))
            prog_err("invalid <amt>");
        if (amt <= 0)
            prog_err("<amt> must be positive");
        if (amt > REC_AMT_MAX)
            prog_err("<amt> out of range");
        amt *= sign;

        // parse date
        int32_t dt;
        if (argc < 3)
            dt = dt_today();
        else if (!prog_parsedt(argv[2], &dt))
            prog_err("invalid <date>");

        // init
        if (NULL == rec_init(&rec, dt, amt, cat, desc))
            prog_err("cannot construct record");
    }

    // log record
    prog_initrl();
    if (rl_count() >= RL_MAXCOUNT)
        prog_err("transaction count already at limit of %td", RL_MAXCOUNT);
    rl_insert(&rec);
    prog_writerl();

    // print data; skip if not enough mem
    rl_slice(rec.dt, rec.dt);
    if (rt_init())
        rt_print(stdout);
    exit(EXIT_SUCCESS);
}
