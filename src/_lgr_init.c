#include <stdio.h>
#include <string.h>

#include "util.h"
#include "program.h"

#define HELP "\
Initialize a new lgr directory from the current directory.\n\
Usage: " PROG_NAME " init\n\
"


void main_init(int argc, char** argv)
{
    for (int i = PROG_ARGSTART; i < argc; i++)
        if (strcmp(argv[i], "-h") == 0)
            prog_pexit(HELP);

    if (util_fexists(PROG_IDFN))
        prog_err("current directory already a lgr directory");

    FILE* f = fopen(PROG_IDFN, "w");
    if (f == NULL)
        prog_err("cannot create file '" PROG_IDFN "'");
    fclose(f);
    prog_pexit("Initialized lgr directory.");
}
