#pragma once
#include "model.h"

void controller_run(GUIModel *m);
void controller_toggle_tool(GUIModel *m, int tool_idx);
void controller_set_min_severity(GUIModel *m, Severity sev);
void controller_select(GUIModel *m, int idx);
void controller_scroll(GUIModel *m, int delta);
