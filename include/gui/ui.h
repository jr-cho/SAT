#pragma once
#include "model.h"

#define WIN_W 1200
#define WIN_H  700

void ui_init(void);
bool ui_frame(GUIModel *m);
void ui_shutdown(void);
