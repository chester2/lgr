/* Record tree used to print logged transactions. */

#ifndef LGR_RECORDTREE_H
#define LGR_RECORDTREE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "recordlist.h"

/* Day indices start at this number. */
#define RT_UI_FIRST_DIND 1

/* Initialize using record list. Record list must already be initialized
and the active slice must be nonempty. Return NULL if insufficient memory. */
bool rt_init(void);

void rt_deinit(void);

/* Print records to STREAM. Return number of lines printed. */
ptrdiff_t rt_print(FILE* stream);

#endif
