#pragma once

#include "correlation.h"
#include "scoring.h"

typedef struct {
    FindingList       *findings;
    CorrelationResult *correlation;
    char             **files;
    u32                file_count;
} Database;

Database    *db_new(void);
void         db_free(Database *db);
int          db_add_finding(Database *db, Finding *f);
void         db_correlate(Database *db);

FindingList *db_query(const Database *db, ToolID tool, Severity min_sev);
