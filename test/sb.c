// code testing sandbox

#include <stdio.h>
#include <string.h>

#include "util.h"


int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    char a[] = "xyz\0efghijklmnopqrstuvw";
    char* b = "ab\0xy";
    char*c = strstr(a, b+3);
    printf("%s\n", strstr(a, b+3));
}