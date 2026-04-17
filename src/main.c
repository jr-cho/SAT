#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/runner/cppcheck.h"
#include "../include/runner/flawfinder.h"
#include "../include/runner/gcc.h"
#include "../include/runner/converity.h"
#include "../include/parser/cppcheck_parser.h"
#include "../include/parser/flawfinder_parser.h"
#include "../include/parser/gcc_parser.h"
#include "../include/parser/coverity_parser.h"
#include "../include/core/database.h"
#include "../include/report/report.h"
#include "../include/report/json.h"
#include "tool_finder.h"

#define OUT_CPPCHECK   "cppcheck_report.xml"
#define OUT_FLAWFINDER "flawfinder_report.txt"
#define OUT_GCC        "gcc_report.txt"
#define OUT_COVERITY   "coverity_report.txt"
#define OUT_REPORT     "analysis_report.txt"
#define OUT_JSON       "analysis_report.json"

static void ingest(Database *db, FindingList *list)
{
    if (!list) return;
    for (size_t i = 0; i < list->count; i++)
        db_add_finding(db, list->items[i]);
    free(list->items);
    free(list);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    const char *target = argv[1];

    printf("Running cppcheck...\n");
    run_cppcheck(target, OUT_CPPCHECK);

    printf("Running flawfinder...\n");
    run_flawfinder(target, OUT_FLAWFINDER);

    const char *gcc_exec = find_gcc_analyzer();
    if (gcc_exec) {
        printf("Running gcc -fanalyzer...\n");
        run_gcc_analyzer(target, OUT_GCC);
    } else {
        printf("gcc -fanalyzer not found, skipping.\n");
    }

    printf("Running coverity...\n");
    int cov_ran = (run_coverity(target, OUT_COVERITY) >= 0);

    Database *db = db_new();
    if (!db) { fprintf(stderr, "Out of memory\n"); return 1; }

    ingest(db, parse_cppcheck(OUT_CPPCHECK));
    ingest(db, parse_flawfinder(OUT_FLAWFINDER));
    if (gcc_exec)
        ingest(db, parse_gcc(OUT_GCC));
    if (cov_ran)
        ingest(db, parse_coverity(OUT_COVERITY));

    db_correlate(db);

    // Generate Report
    printf("\n");
    report_print(db, target, stdout);

    if (report_write(db, target, OUT_REPORT) == 0)
        printf("\nReport saved to %s\n", OUT_REPORT);
    else
        fprintf(stderr, "Failed to write %s\n", OUT_REPORT);

    if (report_write_json(db, target, OUT_JSON) == 0)
        printf("Report saved to %s\n", OUT_JSON);
    else
        fprintf(stderr, "Failed to write %s\n", OUT_JSON);

    remove(OUT_CPPCHECK);
    remove(OUT_FLAWFINDER);
    if (gcc_exec)   remove(OUT_GCC);
    if (cov_ran)    remove(OUT_COVERITY);

    db_free(db);
    return 0;
}
