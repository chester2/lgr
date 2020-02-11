/*
 * Sorted array of records.
 * 
 * Only a single object exists. It is statically allocated and exists for
 * the lifetime of the program.
 */

#ifndef LGR_RECORDLIST_H
#define LGR_RECORDLIST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "record.h"

#define RL_MAXCOUNT (PTRDIFF_MAX / 2)

/* Record access. Return NULL if INDEX is out of bounds. */
const Record* rl_get(ptrdiff_t index);

/* Getters. */
ptrdiff_t rl_count(void);
ptrdiff_t rl_slicestart(void);
ptrdiff_t rl_slicestop(void);
ptrdiff_t rl_slicecount(void);

/*
 * Initialize record list by reading a file.
 * 
 * Enough space is allocated for a single insertion. Active slice is set to the entire list.
 * 
 * Parameters
 * ----------
 * f
 *      Allows reading (will start from beginning of file). May also be
 *      NULL (interpreted as a blank file).
 * 
 * Returns
 * -------
 * 0 on success.
 * Negative if insufficient memory.
 * PTRDIFF_MAX if too many lines.
 * Positive line number if deserializing that line failed.
 */
ptrdiff_t rl_init(FILE* f);

/* Serialize to file. Writing will begin wherever the current write
position is. */
void rl_write(FILE* f);

/* Deallocate. */
void rl_deinit(void);

/* Copy-insert a valid record. Automatically adjust slice boundaries.
Return the inserted record. */
const Record* rl_insert(const Record* rec);

/* Delete record. Automatically adjust slice boundaries. Return false if
INDEX is out of bounds. */
bool rl_delete(ptrdiff_t index);

/* Reset record list's slice to the entire list. Return record count. */
ptrdiff_t rl_resetslice(void);

/* Set the active slice such that all slice records have dates D satisfying
`dt0` <= D <= `dt1`. Return resultant slice count. */
ptrdiff_t rl_slice(int32_t dt0, int32_t dt1);

/* Delete all active slice records for which none of the given patterns is
a substring. Patterns in PATTERNS are separated by DELIM. Return resultant
slice length, or -1 if there is insufficient memory. */
ptrdiff_t rl_filtercat(const char* patterns, int delim);
ptrdiff_t rl_filterdesc(const char* patterns, int delim);

#endif
