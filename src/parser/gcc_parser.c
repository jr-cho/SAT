#include "../../include/parser/gcc_parser.h"
#include "../../include/common/utils.h"
#include "../../include/common/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * GCC -fanalyzer diagnostic line format:
 *   <file>:<line>:<col>: warning: <message> [CWE-NNN] [-Wanalyzer-flag]
 *   <file>:<line>:<col>: error:   <message>
 *   <file>:<line>:<col>: note:    <message>
 *
 * All other lines (code context, event traces, arrows) are skipped.
 */

static Severity sev_from_gcc(const char *type)
{
    if (strcmp(type, "error")   == 0) return SEV_HIGH;
    if (strcmp(type, "warning") == 0) return SEV_MEDIUM;
    if (strcmp(type, "note")    == 0) return SEV_INFO;
    return SEV_UNKNOWN;
}

/*
 * Try to parse a primary diagnostic line into its components.
 * Returns 1 on success, 0 if the line does not match the format.
 */
static int parse_diag_line(const char *line,
                            char *file,  size_t file_size,
                            int  *lineno,
                            int  *colno,
                            char *sev,   size_t sev_size,
                            char *msg,   size_t msg_size)
{
    /* File ends at the first ':' that is followed by a digit. */
    const char *p = line;
    const char *colon1 = NULL;
    while (*p) {
        if (*p == ':' && *(p + 1) >= '0' && *(p + 1) <= '9') {
            colon1 = p;
            break;
        }
        p++;
    }
    if (!colon1) return 0;

    /* Line number */
    char *end;
    long lno = strtol(colon1 + 1, &end, 10);
    if (end == colon1 + 1 || *end != ':') return 0;

    /* Column number */
    long cno = strtol(end + 1, &end, 10);
    if (*end != ':') return 0;

    /* ": <sev>: " */
    const char *after_col = end + 1;
    if (*after_col != ' ') return 0;
    after_col++;

    static const char *types[] = { "warning: ", "error: ", "note: ", NULL };
    const char *matched_type = NULL;
    size_t type_len = 0;
    for (int i = 0; types[i]; i++) {
        size_t tl = strlen(types[i]);
        if (strncmp(after_col, types[i], tl) == 0) {
            matched_type = types[i];
            type_len = tl;
            break;
        }
    }
    if (!matched_type) return 0;

    /* Copy file path */
    size_t flen = (size_t)(colon1 - line);
    if (flen >= file_size) flen = file_size - 1;
    memcpy(file, line, flen);
    file[flen] = '\0';

    *lineno = (int)lno;
    *colno  = (int)cno;

    /* Severity string (strip trailing " ") */
    size_t slen = type_len - 2; /* drop ": " */
    if (slen >= sev_size) slen = sev_size - 1;
    memcpy(sev, matched_type, slen);
    sev[slen] = '\0';

    /* Message — everything after "sev: " */
    const char *msg_start = after_col + type_len;
    size_t mlen = strlen(msg_start);
    if (mlen > 0 && msg_start[mlen - 1] == '\n') mlen--;
    if (mlen >= msg_size) mlen = msg_size - 1;
    memcpy(msg, msg_start, mlen);
    msg[mlen] = '\0';

    return 1;
}

/*
 * Extract "CWE-NNN" from a message like "... [CWE-476] [-Wflag]".
 * Returns a malloc'd string, or NULL.  Also trims the annotations from msg.
 */
static char *extract_cwe(char *msg)
{
    char *cwe = NULL;

    char *start = strstr(msg, "[CWE-");
    if (start) {
        char *close = strchr(start, ']');
        if (close) {
            size_t len = (size_t)(close - start - 1); /* skip '[' */
            cwe = malloc(len + 1);
            if (cwe) {
                memcpy(cwe, start + 1, len);
                cwe[len] = '\0';
            }
        }
        /* Truncate message before " [CWE-..." */
        if (start > msg && *(start - 1) == ' ') start--;
        *start = '\0';
    }

    /* Strip any remaining " [-Wflag]" suffix */
    char *flag = strstr(msg, " [-W");
    if (flag) *flag = '\0';

    return cwe;
}

FindingList *parse_gcc(const char *output_file)
{
    FILE *fp = fopen(output_file, "r");
    if (!fp) {
        LOG_ERROR("Cannot open GCC output: %s", output_file);
        return NULL;
    }

    FindingList *list = finding_list_new();
    if (!list) { fclose(fp); return NULL; }

    char line[4096];

    while (fgets(line, sizeof(line), fp)) {
        char file[1024], sev[32], msg[2048];
        int  lineno, colno;

        if (!parse_diag_line(line, file, sizeof(file),
                             &lineno, &colno,
                             sev, sizeof(sev),
                             msg, sizeof(msg)))
            continue;

        Finding *f = finding_new();
        if (!f) continue;

        f->tool     = TOOL_GCC;
        f->file     = str_dup(file);
        f->line     = lineno;
        f->column   = colno;
        f->severity = sev_from_gcc(sev);
        f->category = extract_cwe(msg);   /* may be NULL — that is fine */
        f->message  = str_dup(msg);

        finding_list_add(list, f);
    }

    fclose(fp);
    LOG_INFO("gcc: parsed %zu findings from %s", list->count, output_file);
    return list;
}
