#include <stdio.h>
#include <string.h>

#include "program.h"

#ifdef _WIN32
#include <windows.h>

static UINT st_origcp;

void reset_cp(void)
{
    fflush(stdout);
    SetConsoleOutputCP(st_origcp);
}
#endif // _WIN32

#define HELP "\
Canadian cash flow tracker.\n\
Usage: " PROG_NAME " <command>\n\
\n\
Commands:\n\
    init    Initialize a new lgr directory.\n\
    log     Log a transaction.\n\
    view    View transactions.\n\
    rm      Remove a transaction.\n\
    sum     Get totals by category.\n\
    plot    Plot monthly totals.\n\
    lim     Work with limits for registered accounts.\n\
"

// The below functions must call exit().
void main_init(int argc, char** argv);
void main_log(int argc, char** argv);
void main_view(int argc, char** argv);
void main_rm(int argc, char** argv);
void main_sum(int argc, char** argv);
void main_plot(int argc, char** argv);
void main_lim(int argc, char** argv);

int main(int argc, char** argv)
{
    #ifdef _WIN32
    st_origcp = GetConsoleOutputCP();
    SetConsoleOutputCP(65001);
    atexit(reset_cp);
    #endif // _WIN32

    setvbuf(stdout, NULL, _IOFBF, BUFSIZ);

    const char* cmd = argv[PROG_ARGSTART - 1];
    if (argc < PROG_ARGSTART || strcmp(cmd, "-h") == 0)
        prog_pexit(HELP);

    #define TRY_DELEGATE(cmdname) \
        if (strcmp(cmd, #cmdname) == 0) main_##cmdname(argc, argv)

    TRY_DELEGATE(init);
    TRY_DELEGATE(log);
    TRY_DELEGATE(view);
    TRY_DELEGATE(rm);
    TRY_DELEGATE(sum);
    TRY_DELEGATE(plot);
    TRY_DELEGATE(lim);

    prog_err("invalid command '%s'", cmd);
}
