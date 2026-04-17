#include "../../include/core/findings.h"
#include "../../include/common/utils.h"
#include <stdlib.h>
#include <string.h>

static Finding *finding_dup(const Finding *src) {
    if (!src) return NULL;
    Finding *dst = finding_new();
    if (!dst) return NULL;
    dst->tool     = src->tool;
    dst->line     = src->line;
    dst->column   = src->column;
    dst->severity = src->severity;
    dst->file     = src->file     ? str_dup(src->file)     : NULL;
    dst->category = src->category ? str_dup(src->category) : NULL;
    dst->message  = src->message  ? str_dup(src->message)  : NULL;
    return dst;
}

Finding *finding_list_get(const FindingList *list, u32 index) {
    if (!list || index >= (u32)list->count) return NULL;
    return list->items[index];
}

FindingList *finding_list_filter_tool(const FindingList *list, ToolID tool) {
    if (!list) return NULL;
    FindingList *out = finding_list_new();
    if (!out) return NULL;

    for (size_t i = 0; i < list->count; i++) {
        Finding *f = list->items[i];
        if (f->tool != tool) continue;

        Finding *copy = finding_dup(f);
        if (!copy || finding_list_add(out, copy) < 0) {
            finding_free(copy);
            finding_list_free(out);
            return NULL;
        }
    }
    return out;
}

FindingList *finding_list_filter_severity(const FindingList *list, Severity min) {
    if (!list) return NULL;
    FindingList *out = finding_list_new();
    if (!out) return NULL;

    for (size_t i = 0; i < list->count; i++) {
        Finding *f = list->items[i];
        if (f->severity < min) continue;

        Finding *copy = finding_dup(f);
        if (!copy || finding_list_add(out, copy) < 0) {
            finding_free(copy);
            finding_list_free(out);
            return NULL;
        }
    }
    return out;
}
