#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "date.h"
#include "entry.h"


struct EntryObject {
    int32_t dt;
    int64_t amt;
    char cat[32];
    char note[64];
};

size_t const ENTRY_SIZE = sizeof(Entry);

/* Max length of a string serialization, including \0.
10 : yyyy-mm-dd
1  : TAB
26 : -92,233,720,368,547,758.08
1  : TAB
31 : category
1  : TAB
63 : note
1  : \0
*/
// static size_t const ENTRY_MAX = 10 + 1 + 26 + 1 + 31 + 1 + 63 + 1;


// Entry Entry_Deserialize(char* s)
// {
//     char delim[] = "\t";
//     Entry entry;

//     /* Clone S. */
//     size_t slen = strlen(s) + 1;
//     char* scpy = malloc(slen);
//     memcpy(scpy, s, slen);

//     /* Parse date. */
//     char* dt = strtok(scpy, delim);
//     entry.dt = dt_stoo(dt);

//     char* amt = strtok(NULL, delim);

//     free(scpy);
// }