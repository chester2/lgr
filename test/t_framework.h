#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define STR_EQ(s1, s2) (strcmp((s1), (s2)) == 0)

bool TFWK_LOG;

void log_intro(const char* s)
{
    char* divider = "---------------------------";
    if (TFWK_LOG) printf("%s\n%s\n%s\n", divider, s, divider);
}

void log_cycle(const char* format, ...)
{
    if (!TFWK_LOG) return;
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    putc('\n', stdout);
}

void log_end(void)
{
    if (TFWK_LOG) putc('\n', stdout);
}
