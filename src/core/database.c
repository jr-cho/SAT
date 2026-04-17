#include "../../include/core/database.h"
#include "../../include/common/utils.h"
#include "../../include/common/logging.h"
#include <stdlib.h>
#include <string.h>

Database *db_new(void) {
    Database *db = calloc(1, sizeof(Database));
    if (!db) return NULL;

    db->findings = finding_list_new();
    if (!db->findings) {
        free(db);
        return NULL;
    }
    return db;
}

void db_free(Database *db) {
    if (!db) return;
    finding_list_free(db->findings);
    correlation_result_free(db->correlation);
    for (u32 i = 0; i < db->file_count; i++)
        free(db->files[i]);
    free(db->files);
    free(db);
}

int db_add_finding(Database *db, Finding *f) {
    if (!db || !f) return -1;
    return finding_list_add(db->findings, f);
}

void db_correlate(Database *db) {
    if (!db) return;
    correlation_result_free(db->correlation);
    db->correlation = correlate(db->findings);
}

FindingList *db_query(const Database *db, ToolID tool, Severity min_sev) {
    if (!db || !db->findings) return NULL;

    FindingList *out = finding_list_new();
    if (!out) return NULL;

    for (size_t i = 0; i < db->findings->count; i++) {
        Finding *f = db->findings->items[i];
        if (tool != TOOL_COUNT && f->tool != tool) continue;
        if (f->severity < min_sev) continue;

        Finding *copy = finding_new();
        if (!copy) goto oom;
        copy->tool     = f->tool;
        copy->line     = f->line;
        copy->column   = f->column;
        copy->severity = f->severity;
        copy->file     = f->file     ? str_dup(f->file)     : NULL;
        copy->category = f->category ? str_dup(f->category) : NULL;
        copy->message  = f->message  ? str_dup(f->message)  : NULL;

        if (finding_list_add(out, copy) < 0) {
            finding_free(copy);
            goto oom;
        }
    }

    LOG_DEBUG("db_query: returned %zu findings (tool=%d, min_sev=%d)",
              out->count, (int)tool, (int)min_sev);
    return out;

oom:
    finding_list_free(out);
    return NULL;
}
