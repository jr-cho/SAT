#include "../../include/core/correlation.h"
#include "../../include/common/logging.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16
#define LINE_WINDOW      3

static int same_issue(const Finding *a, const Finding *b) {
    if (!a->file || !b->file) return 0;
    if (strcmp(a->file, b->file) != 0) return 0;
    if (a->line < 0 || b->line < 0) return 0;
    int diff = a->line - b->line;
    return diff >= -LINE_WINDOW && diff <= LINE_WINDOW;
}

static CorrelationResult *result_new(void) {
    CorrelationResult *r = malloc(sizeof(CorrelationResult));
    if (!r) return NULL;
    r->items = malloc(INITIAL_CAPACITY * sizeof(CorrelatedFinding *));
    if (!r->items) { free(r); return NULL; }
    r->count    = 0;
    r->capacity = INITIAL_CAPACITY;
    return r;
}

static int result_append(CorrelationResult *r, CorrelatedFinding *cf) {
    if (r->count == r->capacity) {
        u32 new_cap = r->capacity * 2;
        CorrelatedFinding **buf = realloc(r->items, new_cap * sizeof(CorrelatedFinding *));
        if (!buf) return -1;
        r->items    = buf;
        r->capacity = new_cap;
    }
    r->items[r->count++] = cf;
    return 0;
}

CorrelationResult *correlate(const FindingList *list) {
    if (!list) return NULL;

    CorrelationResult *result = result_new();
    if (!result) return NULL;

    for (size_t i = 0; i < list->count; i++) {
        Finding *f = list->items[i];

        // Searching for existing group -JG
        CorrelatedFinding *match = NULL;
        for (u32 j = 0; j < result->count; j++) {
            CorrelatedFinding *cf = result->items[j];
            for (int t = 0; t < TOOL_COUNT; t++) {
                if (cf->sources[t] && same_issue(cf->sources[t], f)) {
                    match = cf;
                    break;
                }
            }
            if (match) break;
        }

        if (!match) {
            match = calloc(1, sizeof(CorrelatedFinding));
            if (!match || result_append(result, match) < 0) {
                free(match);
                correlation_result_free(result);
                return NULL;
            }
        }

        if (!match->sources[f->tool]) {
            match->sources[f->tool] = f;
            match->tool_mask |= (1u << (u32)f->tool);
            if (f->severity > match->consensus_severity)
                match->consensus_severity = f->severity;
        }
    }

    LOG_INFO("correlation: %zu findings -> %u groups", list->count, result->count);
    return result;
}

void correlation_result_free(CorrelationResult *result) {
    if (!result) return;
    for (u32 i = 0; i < result->count; i++)
        free(result->items[i]);
    free(result->items);
    free(result);
}
