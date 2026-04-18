#pragma once
#include "../core/database.h"
#include <stdbool.h>
#include <pthread.h>

typedef enum { GUI_IDLE, GUI_RUNNING, GUI_DONE, GUI_ERROR } GUIState;

typedef struct {
    Database        *db;
    char             target[512];
    bool             input_focused;

    GUIState         state;
    char             status_msg[256];
    pthread_t        worker;
    pthread_mutex_t  lock;

    bool             tool_enabled[TOOL_COUNT];
    Severity         min_severity;

    /* filtered view: indices into db->findings->items */
    int             *visible_idx;
    int              visible_count;
    int              selected;       /* index into visible_idx, -1 = none */
    int              scroll;
    int              rows_visible;
} GUIModel;

void     model_init(GUIModel *m);
void     model_destroy(GUIModel *m);
void     model_rebuild_visible(GUIModel *m);
Finding *model_get_visible(const GUIModel *m, int i);
Finding *model_get_selected(const GUIModel *m);
