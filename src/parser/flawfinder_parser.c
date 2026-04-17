#include "../../include/parser/flawfinder_parser.h"
#include "../../include/common/utils.h"
#include "../../include/common/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Parses one CSV field from *pos, writes it into buf, and advances *pos
 * past the trailing comma.  Handles RFC-4180 double-quote escaping ("").
 * Returns 1 if a field was consumed, 0 at end-of-line.
 */
static int csv_next_field(const char **pos, char *buf, size_t buf_size)
{
    const char *p = *pos;
    if (*p == '\0' || *p == '\n' || *p == '\r') return 0;

    size_t i = 0;
    if (*p == '"') {
        p++;
        while (*p && *p != '\n') {
            if (*p == '"') {
                p++;
                if (*p == '"') {          /* escaped "" → single " */
                    if (i < buf_size - 1) buf[i++] = '"';
                    p++;
                } else {
                    break;                /* closing quote */
                }
            } else {
                if (i < buf_size - 1) buf[i++] = *p;
                p++;
            }
        }
    } else {
        while (*p && *p != ',' && *p != '\n' && *p != '\r') {
            if (i < buf_size - 1) buf[i++] = *p;
            p++;
        }
    }
    buf[i] = '\0';

    if (*p == ',') p++;
    *pos = p;
    return 1;
}

/*
 * Expected CSV columns (flawfinder --dataonly --csv):
 *   0  File
 *   1  Line
 *   2  Column
 *   3  DefaultLevel  (ignored)
 *   4  Level         (risk 0-5)
 *   5  Category
 *   6  Name          (function / pattern)
 *   7  Warning       (description)
 *   8  Suggestion    (ignored)
 *   9  Note          (ignored)
 *  10  CWEs
 *  11  Context       (ignored)
 */
static Severity level_to_severity(int level)
{
    if (level >= 5) return SEV_CRITICAL;
    if (level == 4) return SEV_HIGH;
    if (level == 3) return SEV_MEDIUM;
    if (level == 2) return SEV_LOW;
    if (level == 1) return SEV_INFO;
    return SEV_UNKNOWN;
}

FindingList *parse_flawfinder(const char *output_file)
{
    FILE *fp = fopen(output_file, "r");
    if (!fp) {
        LOG_ERROR("Cannot open flawfinder output: %s", output_file);
        return NULL;
    }

    FindingList *list = finding_list_new();
    if (!list) { fclose(fp); return NULL; }

    char line[4096];
    int  first_line = 1;

    while (fgets(line, sizeof(line), fp)) {
        if (first_line) { first_line = 0; continue; } /* skip header row */

        const char *pos = line;
        char fields[12][512];
        int  n = 0;

        while (n < 12 && csv_next_field(&pos, fields[n], sizeof(fields[n])))
            n++;

        if (n < 8) continue; /* not enough fields */

        Finding *f = finding_new();
        if (!f) continue;

        f->tool     = TOOL_FLAWFINDER;
        f->file     = str_dup(fields[0]);
        f->line     = atoi(fields[1]);
        f->column   = atoi(fields[2]);
        f->severity = level_to_severity(atoi(fields[4]));

        /* Category: "Name (Category)" e.g. "gets (buffer)" */
        char cat_buf[1030];
        snprintf(cat_buf, sizeof(cat_buf), "%s (%s)", fields[6], fields[5]);
        f->category = str_dup(cat_buf);

        /* Message: warning text, optionally prefixed with CWE */
        if (n >= 11 && fields[10][0] != '\0') {
            char msg_buf[1030];
            snprintf(msg_buf, sizeof(msg_buf), "[%s] %s", fields[10], fields[7]);
            f->message = str_dup(msg_buf);
        } else {
            f->message = str_dup(fields[7]);
        }

        finding_list_add(list, f);
    }

    fclose(fp);
    LOG_INFO("flawfinder: parsed %zu findings from %s", list->count, output_file);
    return list;
}
