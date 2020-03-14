#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "date.h"
#include "recordlist.h"
#include "recordtree.h"
#include "program.h"

#define HELP "\
Remove a transaction.\n\
Usage: " PROG_NAME " rm [<date> [<index]]\n\
\n\
    <date>      'dN' or 'yyyy-mm-dd'.\n\
    <index>     Transaction index within <date>. If omitted, remove <date>'s\n\
                latest transaction.\n\
"

int main_rm(int argc, char** argv)
{
    argc -= PROG_ARGSTART;
    argv += PROG_ARGSTART;
    for (int i = 0; i < argc; i++)
        if (strcmp(argv[i], "-h") == 0)
            prog_pexit(HELP);
    prog_loadconf();

    // parse date
    int32_t dt;
    if (argc == 0)
        dt = dt_today();
    else if (!prog_parsedt(argv[0], &dt))
        prog_err("invalid <date>");

    // parse index
    long long index = -1;   // default value for if index is omitted
    bool status;
    if (argc >= 2) {
        index = util_stoi(argv[1], &status) - RT_UI_FIRST_DIND;
        if (!status)
            prog_err("invalid <index>");
        if (index < 0)
            prog_err("<index> out of bounds");
    }

    // delete
    prog_initrl();
    ptrdiff_t slicelen = rl_slice(dt, dt);
    if (slicelen == 0)
        prog_err("no transactions to remove");
    if (index >= slicelen)
        prog_err("<index> out of bounds");
    index += (index == -1) ? rl_slicestop() : rl_slicestart();
    rl_delete(index);
    prog_writerl();

    // print data; skip if not enough mem
    if (0 == rl_slicestop() - rl_slicestart()) {
        puts(dt_fmt(dt));
        prog_pexit("No more records.");
    } else if (rt_init()) {
        rt_print(stdout);
    }
    exit(EXIT_SUCCESS);
}
