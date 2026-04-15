#include "../../include/parser/parser.h"
#include "../../include/common/utils.h"
#include <stdlib.h>

#define INITIAL_CAPACITY 16

FindingList *finding_list_new(void)
{
    FindingList *list = malloc(sizeof(FindingList));
    if (!list) return NULL;

    list->items = malloc(INITIAL_CAPACITY * sizeof(Finding *));
    if (!list->items) {
        free(list);
        return NULL;
    }

    list->count    = 0;
    list->capacity = INITIAL_CAPACITY;
    return list;
}

int finding_list_add(FindingList *list, Finding *f)
{
    if (list->count == list->capacity) {
        size_t new_cap = list->capacity * 2;
        Finding **new_items = realloc(list->items, new_cap * sizeof(Finding *));
        if (!new_items) return -1;
        list->items    = new_items;
        list->capacity = new_cap;
    }

    list->items[list->count++] = f;
    return 0;
}

void finding_list_free(FindingList *list)
{
    if (!list) return;
    for (size_t i = 0; i < list->count; i++)
        finding_free(list->items[i]);
    free(list->items);
    free(list);
}
