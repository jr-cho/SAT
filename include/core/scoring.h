#pragma once

#include "../common/types.h"

Severity normalize_severity(ToolID tool, const char *raw);

float confidence_score(u32 tool_mask);
