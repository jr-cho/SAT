#include "../../include/gui/model.h"
#include <string.h>
#include <stdlib.h>

void model_init(GUIModel *m)
{
    memset(m, 0, sizeof(*m));
    m->db    = db_new();
    m->state = GUI_IDLE;
    for (int i = 0; i < TOOL_COUNT; i++) m->tool_enabled[i] = true;
    m->min_severity = SEV_UNKNOWN;
    m->selected     = -1;
    m->rows_visible = 11;
    mtx_init(&m->lock, mtx_plain);
    snprintf(m->status_msg, sizeof(m->status_msg),
             "Enter a source file path and click Run Analysis");
}

void model_destroy(GUIModel *m)
{
    if (m->state == GUI_RUNNING)
        thrd_join(m->worker, NULL);
    free(m->visible_idx);
    db_free(m->db);
    mtx_destroy(&m->lock);
}

void model_rebuild_visible(GUIModel *m)
{
    free(m->visible_idx);
    m->visible_idx   = NULL;
    m->visible_count = 0;

    if (!m->db || !m->db->findings) return;

    int n = (int)m->db->findings->count;
    if (n == 0) return;

    m->visible_idx = malloc((size_t)n * sizeof(int));
    if (!m->visible_idx) return;

    for (int i = 0; i < n; i++) {
        Finding *f = m->db->findings->items[i];
        if (!m->tool_enabled[f->tool]) continue;
        if (f->severity < m->min_severity) continue;
        m->visible_idx[m->visible_count++] = i;
    }

    if (m->selected >= m->visible_count)
        m->selected = m->visible_count > 0 ? 0 : -1;
    m->scroll = 0;
}

Finding *model_get_visible(const GUIModel *m, int i)
{
    if (i < 0 || i >= m->visible_count || !m->db || !m->db->findings) return NULL;
    return m->db->findings->items[m->visible_idx[i]];
}

Finding *model_get_selected(const GUIModel *m)
{
    return model_get_visible(m, m->selected);
}
