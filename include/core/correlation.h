#pragma once

#include "findings.h"

typedef struct {
    Finding *sources[TOOL_COUNT];
    u32      tool_mask;
    Severity consensus_severity;
} CorrelatedFinding;

typedef struct {
    CorrelatedFinding **items;
    u32                 count;
    u32                 capacity;
} CorrelationResult;

CorrelationResult *correlate(const FindingList *list);
void               correlation_result_free(CorrelationResult *result);
