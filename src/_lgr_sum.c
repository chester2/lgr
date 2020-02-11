#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "date.h"
#include "record.h"
#include "hashtable.h"
#include "recordlist.h"
#include "program.h"

#define CMD PROG_NAME " sum"

#define HELP "\
Sum transaction amounts by category in the given time range.\n\
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

int main_sum(int argc, char** argv)
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
        void printsums(void);
        printsums();
    }
    exit(EXIT_SUCCESS);
}


// Printing

/*
 * _______ __________ __________
 *  In    | work     | 1,600.02
 *        | misc     |     0.44
 * _______|__________|__________
 *  Out   | gas      |    26.10
 *        | snacks   |     1.33
 * _______|__________|__________
 *  Total | In       | 1,600.46
 *        | Out      |    27.43
 *        | Net      | 1,573.03
 * \______/\_________/\________/
 *     S        C          A
 * 
 * C cat length: MAX(sizeof "Category", sizeof longest category)
 * A amt length: MAX(sizeof "Amount", amtlen(total in), amtlen(total out))
 */

static void putchars(int c, int count)
{
    for (int i = 0; i < count; i++) putc(c, stdout);
}

/*
 * Print any line (except the first line which requies manual printing).
 * 
 * To print a divider, pass NULL for SECTNAME, CAT, and AMTBUF. CATLEN and
 * AMTLEN must still be set properly.
 */
static void printline(
    const char* sectname,
    const char* cat, int catlen,
    int64_t amt, int amtlen, char* amtbuf
) {
    if (sectname == NULL)
        fputs("_______|", stdout);
    else if (sectname[0] == '\0')
        fputs("       |", stdout);
    else
        printf(" %-5s |", sectname);

    if (cat) {
        putc(' ', stdout);
        putchars(' ', catlen - printf(cat));
        fputs(" |", stdout);
    } else {
        putc('_', stdout);
        putchars('_', catlen);
        fputs("_|", stdout);
    }

    if (amtbuf) {
        putc(' ', stdout);
        int i = util_fmtcents(amt, amtbuf);
        amtbuf[i] = 0;
        putchars(' ', amtlen - i);
        puts(amtbuf);
    } else {
        putc('_', stdout);
        putchars('_', amtlen);
        puts("_");
    }
}

/* Hashtable wrapper to encapsulate useful printing information. */
typedef struct {
    HashTable* ht;       // category totals for a given sign
    const char* name;   // "In"/"Out" or equivalent combination
    int64_t max;        // maximum category total, not absolute value
    int64_t total;      // total over all categories, not absolute value
    int sign;           // must be 1 or -1
} Section;

/* Exits on insufficient memory. The name member is set to NAME (name
argument is NOT copied). */
static Section* initsect(Section* sect, const char* name, int sign)
{
    sect->ht = ht_new(HT_STR);
    if (sect->ht == NULL)
        prog_err_nomem();

    for (
        ptrdiff_t i = rl_slicestart()
        ; i < rl_slicestop()
        ; i++
    ) {
        const Record* rec = rl_get(i);
        if (sign * rec->amt >= 0) {
            int64_t* catsum = ht_insert(sect->ht, rec->cat, 0);
            if (catsum == NULL)
                prog_err_nomem();
            *catsum += rec->amt;
        }
    }

    ht_sort(sect->ht, false, sign < 0);
    sect->max = ht_max(sect->ht);
    sect->total = ht_sum(sect->ht);
    sect->name = name;
    sect->sign = sign;
    return sect;
}

/* Exit program on error. */
void printsums(void)
{
    // compute tsigns
    enum {POS, NEG, NSECTIONS};
    Section sects[NSECTIONS];
    initsect(sects + POS, "In", 1);
    initsect(sects + NEG, "Out", -1);

    // get net
    int64_t net = sects[POS].total + sects[NEG].total;

    // get catlen
    int catlen = 0;
    for (
        ptrdiff_t i = rl_slicestart()
        ; i < rl_slicestop()
        ; i++
    ) catlen = util_max(catlen, strlen(rl_get(i)->cat));

    // get amtlen and buffer
    int amtlen = util_max(
        util_fmtcentslen(net),
        util_max(
            util_fmtcentslen(sects[POS].total),
            util_fmtcentslen(sects[NEG].total) - 1
        )
    );
    char amtbuf[sizeof "-92,233,720,368,547,758.08"];

    // line 1
    putchars('_', 9 + catlen + 3 + amtlen + 1);
    putc('\n', stdout);

    // print In/Out sections
    for (int i = 0; i < NSECTIONS; i++) {
        Section* sect = sects + i;
        if (sect->total == 0) continue;
        const char* sectname = sect->name;
        const void* key;
        int64_t value;
        ht_foreach(sect->ht, key, &value) {
            printline(sectname, (char*)key, catlen, sect->sign*value, amtlen, amtbuf);
            sectname = "";
        }
        printline(NULL, NULL, catlen, 0, amtlen, NULL);
    }

    // print Total section
    // printline(NULL, NULL, catlen, 0, amtlen, NULL);
    const char* sectname = "Total";
    for (int i = 0; i < NSECTIONS; i++) {
        Section* sect = sects + i;
        if (sect->total == 0) continue;
        int64_t abstot = sect->sign * sect->total;
        printline(sectname, sect->name, catlen, abstot, amtlen, amtbuf);
        sectname = "";
    }
    printline(sectname, "Net", catlen, net, amtlen, amtbuf);
    putc('\n', stdout);
}
