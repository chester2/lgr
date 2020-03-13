#ifndef LGR_PROGRAM_H
#define LGR_PROGRAM_H

#include <stdbool.h>
#include <stdint.h>

#include "recordlist.h"

#define PROG_NAME "lgr"
#define PROG_IDFN "." PROG_NAME
#define PROG_DATAFN PROG_NAME "_data.tsv"
#define PROG_LIMFN PROG_NAME "_limits.ini"
#define PROG_ARGSTART 2

#define PROG_CONF_LOG_SIGN "log_sign"
#define PROG_CONF_LIM_TYPE "lim_type"

/* Read PROG_IDFN and load config options. Config file must consist only of
lines in the form "key=value", where values are interpreted as integers.
Exit program if PROG_IDFN does not exist or can't be read. */
void prog_loadconf(void);

/* Assume option exists. */
int64_t prog_getconf(const char* option);

/* Print a message and exit. */
void prog_pexit(const char* s);

/* Printf an error message and exit. A newline is automatically appended. */
void prog_err(const char* format, ...);

/* Standard program exit error messages. */
void prog_err_nomem(void);
void prog_err_optnoval(int opt);
void prog_err_optunknown(int opt);
void prog_err_missingargs(void);
void prog_err_read(const char* fp);
void prog_err_write(const char* fp);

/* Parse numbers, ISO dates, or text of the form 'x', 'xn', 'x+n', and
'x-n'. */
bool prog_parsey(const char* s, int* y);
bool prog_parsem(const char* s, int* m);
bool prog_parsedt(const char* s, int32_t* dt);

/* Parse monetary amounts into cents. Input string must be have at most 2
decimal places. Supports using commas as thousands separators. */
bool prog_parsecents(const char* s, int64_t* cents);

/* Initilialize record list. Exit program on error. */
void prog_initrl(void);

/* Write record list. Exit program on error. */
void prog_writerl(void);

/* Print "mmm d, yyyy -- mmm d, yyyy\n". */
void prog_printdaterange(int32_t dt0, int32_t dt1);

#endif
