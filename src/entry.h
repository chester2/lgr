#ifndef ENTRY_H
#define ENTRY_H

#include <stdio.h>
#include <stdint.h>

typedef struct EntryObject Entry;

Entry Entry_Deserialize(char* s);

#endif