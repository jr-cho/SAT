#pragma once

#include "../parser/parser.h"

Finding     *finding_list_get(const FindingList *list, u32 index);
FindingList *finding_list_filter_tool(const FindingList *list, ToolID tool);
FindingList *finding_list_filter_severity(const FindingList *list, Severity min);
