#include <stdio.h>

#define REF_FN "_testelist_serialization_ref.txt"
#define REF_CONTENT ( \
    "1999-01-01\t10\tabc\t\n" \
    "1999-01-01\t-20\tabc\t\n" \
    "1999-01-01\t1\txyz\t\n" \
    "1999-01-31\t-100\tdef\tbleh blah\n" \
    "1999-02-02\t700\txyz\t \n" \
    "1999-12-31\t-600\txyz\t\n" \
    "2001-01-02\t51\tdef\tlate\n" \
    "2010-01-02\t-100000000000000\tabcdefghijklmnopqrstuvw\t\n" \
    "2010-12-02\t53\tcat2\t\n" \
    "2010-12-03\t54\tcat3\t\n" \
)

FILE* ref_mkfile(void)
{
    FILE* f = fopen(REF_FN, "w");
    fprintf(f, "%s", REF_CONTENT);
    fclose(f);
    return fopen(REF_FN, "r");
}

void ref_rmfile(FILE* f)
{
    fclose(f);
    remove(REF_FN);
}
