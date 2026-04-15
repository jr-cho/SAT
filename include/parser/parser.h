#ifndef PARSER_H
#define PARSER_H

#include "../common/types.h"
#include <stddef.h>

typedef struct {
    Finding **items;
    size_t    count;
    size_t    capacity;
} FindingList;

FindingList *finding_list_new(void);
int          finding_list_add(FindingList *list, Finding *f);
void         finding_list_free(FindingList *list);

#endif
