// <1> Configuration
// <2> Print And Exit
// <3> Parse Numbers
// <4> Convenience Functions

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"
#include "date.h"
#include "hashtable.h"
#include "recordlist.h"
#include "program.h"


// <1> Configuration

/* Configurations. */
static HashTable* st_conf;

void prog_loadconf(void)
{
    if (!util_fexists(PROG_IDFN))
        prog_err("not a " PROG_NAME " directory");

    // default values
    st_conf = ht_new(HT_STR);
    if (st_conf == NULL)
        prog_err_nomem();
    ht_insert(st_conf, PROG_CONF_LOG_SIGN, 1);

    // read
    FILE* f = fopen(PROG_IDFN, "r");
    if (f == NULL)
        prog_err_read(PROG_IDFN);

    // load
    for (ptrdiff_t lineno = 1;; lineno++) {
        char buf[32];
        char *skey, *svalue;
        {
            int status = util_readiniline(f, sizeof buf, buf, &skey, &svalue);
            if (status == -1)
                break;
            else if (status != 0)
                prog_err("%s:%td: invalid line", PROG_IDFN, lineno);
        }

        int64_t* value = ht_get(st_conf, skey);
        if (value == NULL) {
            continue;
        } else {
            bool status;
            long long tmp = util_stoi(svalue, &status);
            if (!status)
                prog_err("%s:%td: invalid value '%s'", PROG_IDFN, lineno, svalue);
            *value = tmp;
        }
    }

    fclose(f);
}

int64_t prog_getconf(const char* option)
{
    return *ht_get(st_conf, option);
}


// <2> Print And Exit

void prog_pexit(const char* s)
{
    puts(s);
    exit(EXIT_SUCCESS);
}

void prog_err(const char* format, ...)
{
    fputs("error:\n  ", stderr);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    putc('\n', stderr);
    exit(EXIT_FAILURE);
}

void prog_err_nomem(void)
{
    prog_err("not enough memory");
}

void prog_err_optnoval(int opt)
{
    prog_err("value missing for option '-%c'", opt);
}

void prog_err_optunknown(int opt)
{
    prog_err("unknown option '-%c'", opt);
}

void prog_err_missingargs(void)
{
    prog_err("not enough arguments");
}

void prog_err_read(const char* fp)
{
    prog_err("cannot read '%s'", fp);
}

void prog_err_write(const char* fp)
{
    prog_err("cannot write to '%s'", fp);
}


// <3> Parse Numbers

static bool xn(const char* s, int c, int* n)
{
    if (*s != c) return false;
    if (s[1] == '\0') {
        *n = 0;
        return true;
    }
    bool status;
    long long tmp = util_stoi(s + 1, &status);
    if (status && tmp >= INT_MIN && tmp <= INT_MAX) {
        *n = tmp;
        return true;
    }
    return false;
}

bool prog_parsey(const char* s, int* y)
{
    if (xn(s, 'y', y)) {
        *y += dt_gety(dt_today());
        return true;
    }
    bool status;
    long long tmp = util_stoi(s, &status);
    if (status && dt_isy(tmp)) {
        *y = tmp;
        return true;
    }
    return false;
}

bool prog_parsem(const char* s, int* m)
{
    if (xn(s, 'm', m)) {
        *m += dt_getm(dt_today());
        return true;
    }
    bool status;
    long long tmp = util_stoi(s, &status);
    if (status && dt_ism(tmp)) {
        *m = tmp;
        return true;
    }
    return false;
}

bool prog_parsedt(const char* s, int32_t* dt)
{
    int d;
    if (xn(s, 'd', &d))
        *dt = dt_shiftd(dt_today(), d);
    else if (dt_isiso(s))
        *dt = dt_fromiso(s);
    else
        return false;
    return dt_isdt(*dt);
}

bool prog_parsecents(const char* s, int64_t* cents)
{
    char scpy[sizeof "-92,233,720,368,547,758.08"];
    int slen = strlen(s);
    if (slen + 1 > (int)sizeof scpy)
        return false;

    int i = 0, j = 0;
    for (; (i < slen) && (s[i] != '.'); i++)
        if (s[i] != ',')
            scpy[j++] = s[i];
    // at this point, s[i] is either '.' or '\0'
    if (slen - i > 3)
        return false;
    for (int k = 1; k <= 2; k++)
        scpy[j++] = (i + k < slen) ? s[i+k] : '0';
    scpy[j] = '\0';

    bool status;
    long long n = util_stoi(scpy, &status);
    if (status && n >= INT64_MIN && n <= INT64_MAX) {
        *cents = n;
        return true;
    }
    return false;
}


// <4> Convenience Functions

void prog_initrl(void)
{
    FILE* f = NULL;
    if (
        util_fexists(PROG_DATAFN)
        && !(f = fopen(PROG_DATAFN, "r"))
    ) prog_err_read(PROG_DATAFN);
    ptrdiff_t status = rl_init(f);
    if (f) fclose(f);
    if (status == 0)
        return;
    if (status == PTRDIFF_MAX)
        prog_err("number of lines in '" PROG_DATAFN "' exceeds %td", RL_MAXCOUNT);
    if (status > 0)
        prog_err(PROG_DATAFN ":%td: line too long or cannot be deserialized", status);
    if (status < 0)
        prog_err_nomem();
}

void prog_writerl(void)
{
    FILE* f = fopen(PROG_DATAFN, "w");
    if (f == NULL)
        prog_err_write(PROG_DATAFN);
    rl_write(f);
    fclose(f);
}

void prog_printdaterange(int32_t dt0, int32_t dt1)
{
    fputs(dt_fmt(dt0), stdout);
    fputs(" \xe2\x94\x80\xe2\x94\x80 ", stdout);
    puts(dt_fmt(dt1));
}
