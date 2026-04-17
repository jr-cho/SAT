#include "../../include/parser/coverity_parser.h"
#include "../../include/common/utils.h"
#include "../../include/common/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Parses cov-format-errors --text-output.  Each finding spans two or more
 * lines; only the CID line and the first severity event line are used:
 *
 *   <file>:<line>:  CID <id> (#N of M): <Category> (<CHECKER>):
 *   <file>:<line>:  <severity>: <event>: <message>
 *
 * Severity values emitted by Coverity: "high", "medium", "low".
 */

typedef enum {
    COV_IDLE,       /* waiting for a CID line   */
    COV_IN_FINDING  /* waiting for severity line */
} CovState;

static Severity sev_from_coverity(const char *s)
{
    if (strcmp(s, "high")   == 0) return SEV_HIGH;
    if (strcmp(s, "medium") == 0) return SEV_MEDIUM;
    if (strcmp(s, "low")    == 0) return SEV_LOW;
    return SEV_UNKNOWN;
}

/*
 * Extract the checker name from the CID line.
 * Looks for the last "(<CHECKER>):" pattern on the line.
 * Returns a malloc'd string or NULL.
 */
static char *extract_checker(const char *line)
{
    const char *end = strrchr(line, ':');
    if (!end || end == line) return NULL;

    const char *close = end - 1;
    while (close > line && *close == ' ') close--;
    if (*close != ')') return NULL;

    const char *open = close - 1;
    while (open > line && *open != '(') open--;
    if (*open != '(') return NULL;

    size_t len = (size_t)(close - open - 1);
    char *checker = malloc(len + 1);
    if (!checker) return NULL;
    memcpy(checker, open + 1, len);
    checker[len] = '\0';
    return checker;
}

/*
 * Parse the file path and line number from the start of a Coverity line:
 *   <file>:<line>:  ...
 * Returns 1 on success.
 */
static int parse_cov_location(const char *line, char *file, size_t file_size, int *lineno)
{
    /* Find first ':' preceded by digits for the line number. */
    const char *p = line;
    const char *colon1 = NULL;
    while (*p) {
        if (*p == ':' && p > line && *(p + 1) >= '0' && *(p + 1) <= '9') {
            colon1 = p;
            break;
        }
        p++;
    }
    if (!colon1) return 0;

    char *end;
    long lno = strtol(colon1 + 1, &end, 10);
    if (end == colon1 + 1 || *end != ':') return 0;

    size_t flen = (size_t)(colon1 - line);
    if (flen >= file_size) flen = file_size - 1;
    memcpy(file, line, flen);
    file[flen] = '\0';

    *lineno = (int)lno;
    return 1;
}

FindingList *parse_coverity(const char *output_file)
{
    FILE *fp = fopen(output_file, "r");
    if (!fp) {
        LOG_ERROR("Cannot open Coverity output: %s", output_file);
        return NULL;
    }

    FindingList *list = finding_list_new();
    if (!list) { fclose(fp); return NULL; }

    char      line[4096];
    CovState  state    = COV_IDLE;
    Finding  *current  = NULL;

    while (fgets(line, sizeof(line), fp)) {

        if (state == COV_IDLE) {
            if (!strstr(line, " CID ")) continue;

            char file[1024];
            int  lineno;
            if (!parse_cov_location(line, file, sizeof(file), &lineno)) continue;

            current = finding_new();
            if (!current) continue;

            current->tool     = TOOL_COVERITY;
            current->file     = str_dup(file);
            current->line     = lineno;
            current->category = extract_checker(line);
            state = COV_IN_FINDING;

        } else { /* COV_IN_FINDING */
            /* Look for the first "  <sev>: <event>: <msg>" line. */
            static const char *sevs[] = { "high", "medium", "low", NULL };
            int matched = 0;

            for (int i = 0; sevs[i]; i++) {
                /* Pattern: "  <sev>: " somewhere after the location prefix. */
                char needle[32];
                snprintf(needle, sizeof(needle), "  %s: ", sevs[i]);
                const char *p = strstr(line, needle);
                if (!p) continue;

                current->severity = sev_from_coverity(sevs[i]);

                /* Message: everything after "  <sev>: <event>: " */
                const char *msg_start = p + strlen(needle);
                const char *colon = strchr(msg_start, ':');
                if (colon && *(colon + 1) == ' ')
                    msg_start = colon + 2;

                size_t mlen = strlen(msg_start);
                if (mlen > 0 && msg_start[mlen - 1] == '\n') mlen--;
                char *msg = malloc(mlen + 1);
                if (msg) {
                    memcpy(msg, msg_start, mlen);
                    msg[mlen] = '\0';
                }
                current->message = msg;
                matched = 1;
                break;
            }

            if (matched) {
                finding_list_add(list, current);
                current = NULL;
                state   = COV_IDLE;
            } else if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') {
                /* Blank line with no severity event found — discard. */
                finding_free(current);
                current = NULL;
                state   = COV_IDLE;
            }
        }
    }

    /* Flush any pending finding (file ended without a blank line). */
    if (current) {
        finding_list_add(list, current);
    }

    fclose(fp);
    LOG_INFO("coverity: parsed %zu findings from %s", list->count, output_file);
    return list;
}
