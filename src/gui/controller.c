#include "../../include/gui/controller.h"
#include "../../include/runner/cppcheck.h"
#include "../../include/runner/flawfinder.h"
#include "../../include/runner/gcc.h"
#include "../../include/runner/converity.h"
#include "../../include/parser/cppcheck_parser.h"
#include "../../include/parser/flawfinder_parser.h"
#include "../../include/parser/gcc_parser.h"
#include "../../include/parser/coverity_parser.h"
#include "../../include/common/logging.h"
#include "tool_finder.h"
#include <string.h>
#include <stdlib.h>

#define TMP_CPPCHECK    "/tmp/sat_cppcheck.xml"
#define TMP_FLAWFINDER  "/tmp/sat_flawfinder.csv"
#define TMP_GCC         "/tmp/sat_gcc.txt"
#define TMP_COVERITY    "/tmp/sat_coverity.txt"

static void ingest(Database *db, FindingList *list)
{
    if (!list) return;
    for (size_t i = 0; i < list->count; i++)
        db_add_finding(db, list->items[i]);
    free(list->items);
    free(list);
}

static void *analysis_thread(void *arg)
{
    GUIModel *m = arg;

    Database *new_db = db_new();
    if (!new_db) {
        pthread_mutex_lock(&m->lock);
        m->state = GUI_ERROR;
        snprintf(m->status_msg, sizeof(m->status_msg), "Out of memory");
        pthread_mutex_unlock(&m->lock);
        return NULL;
    }

    run_cppcheck(m->target, TMP_CPPCHECK);
    run_flawfinder(m->target, TMP_FLAWFINDER);

    const char *gcc_exec = find_gcc_analyzer();
    if (gcc_exec) run_gcc_analyzer(m->target, TMP_GCC);

    int cov_ran = (run_coverity(m->target, TMP_COVERITY) >= 0);

    ingest(new_db, parse_cppcheck(TMP_CPPCHECK));
    ingest(new_db, parse_flawfinder(TMP_FLAWFINDER));
    if (gcc_exec) ingest(new_db, parse_gcc(TMP_GCC));
    if (cov_ran)  ingest(new_db, parse_coverity(TMP_COVERITY));

    db_correlate(new_db);

    remove(TMP_CPPCHECK);
    remove(TMP_FLAWFINDER);
    if (gcc_exec) remove(TMP_GCC);
    if (cov_ran)  remove(TMP_COVERITY);

    pthread_mutex_lock(&m->lock);
    db_free(m->db);
    m->db = new_db;
    snprintf(m->status_msg, sizeof(m->status_msg),
             "%zu finding%s in %s",
             new_db->findings->count,
             new_db->findings->count == 1 ? "" : "s",
             m->target);
    m->state = GUI_DONE;
    pthread_mutex_unlock(&m->lock);

    return NULL;
}

void controller_run(GUIModel *m)
{
    if (m->state == GUI_RUNNING) return;
    if (m->target[0] == '\0') {
        snprintf(m->status_msg, sizeof(m->status_msg), "No target file specified");
        return;
    }
    m->state = GUI_RUNNING;
    snprintf(m->status_msg, sizeof(m->status_msg),
             "Running analysis on %s...", m->target);
    pthread_create(&m->worker, NULL, analysis_thread, m);
}

void controller_toggle_tool(GUIModel *m, int tool_idx)
{
    if (tool_idx < 0 || tool_idx >= TOOL_COUNT) return;
    m->tool_enabled[tool_idx] = !m->tool_enabled[tool_idx];
    model_rebuild_visible(m);
}

void controller_set_min_severity(GUIModel *m, Severity sev)
{
    m->min_severity = sev;
    model_rebuild_visible(m);
}

void controller_select(GUIModel *m, int idx)
{
    if (idx < 0 || idx >= m->visible_count) idx = -1;
    m->selected = idx;
}

void controller_scroll(GUIModel *m, int delta)
{
    m->scroll += delta;
    int max_scroll = m->visible_count - m->rows_visible;
    if (m->scroll > max_scroll) m->scroll = max_scroll;
    if (m->scroll < 0)          m->scroll = 0;
}
