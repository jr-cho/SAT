#include "../../include/core/scoring.h"
#include <stdlib.h>
#include <string.h>

Severity normalize_severity(ToolID tool, const char *raw) {
    if (!raw) return SEV_UNKNOWN;

    switch (tool) {
        case TOOL_CPPCHECK:
            if (strcmp(raw, "error") == 0)        return SEV_HIGH;
            if (strcmp(raw, "warning") == 0)      return SEV_MEDIUM;
            if (strcmp(raw, "style") == 0)        return SEV_LOW;
            if (strcmp(raw, "performance") == 0)  return SEV_LOW;
            if (strcmp(raw, "portability") == 0)  return SEV_LOW;
            if (strcmp(raw, "information") == 0)  return SEV_INFO;
            return SEV_UNKNOWN;

        case TOOL_FLAWFINDER: {
            // FLAWFINDER uses 1-5
            int level = (int)strtol(raw, NULL, 10);
            if (level >= 5) return SEV_CRITICAL;
            if (level == 4) return SEV_HIGH;
            if (level == 3) return SEV_MEDIUM;
            if (level == 2) return SEV_LOW;
            if (level == 1) return SEV_INFO;
            return SEV_UNKNOWN;
        }

        case TOOL_GCC:
            if (strcmp(raw, "error") == 0)   return SEV_HIGH;
            if (strcmp(raw, "warning") == 0) return SEV_MEDIUM;
            if (strcmp(raw, "note") == 0)    return SEV_INFO;
            return SEV_UNKNOWN;

        case TOOL_COVERITY:
            if (strcmp(raw, "high") == 0)   return SEV_HIGH;
            if (strcmp(raw, "medium") == 0) return SEV_MEDIUM;
            if (strcmp(raw, "low") == 0)    return SEV_LOW;
            return SEV_UNKNOWN;

        default:
            return SEV_UNKNOWN;
    }
}

float confidence_score(u32 tool_mask) {
    // basically says how many times different tools find the same thing
    u32 bits = tool_mask;
    u32 count = 0;
    while (bits) {
        count += bits & 1u;
        bits >>= 1;
    }
    return (float)count / (float)TOOL_COUNT;
}
