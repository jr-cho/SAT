#include "../../include/parser/cppcheck_parser.h"
#include "../../include/common/utils.h"
#include "../../include/common/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Extract the value of an XML attribute from a line.
 * e.g. attr_val(line, "severity") on
 *   <error id="x" severity="error" msg="y">
 * returns a malloc'd "error".
 */
static char *attr_val(const char *line, const char *attr)
{
    char needle[64];
    snprintf(needle, sizeof(needle), "%s=\"", attr);

    const char *start = strstr(line, needle);
    if (!start) return NULL;
    start += strlen(needle);

    const char *end = strchr(start, '"');
    if (!end) return NULL;

    size_t len = (size_t)(end - start);
    char *val = malloc(len + 1);
    if (!val) return NULL;
    memcpy(val, start, len);
    val[len] = '\0';
    return val;
}

static Severity severity_from_cppcheck(const char *s)
{
    if (!s)                         return SEV_UNKNOWN;
    if (strcmp(s, "error") == 0)    return SEV_HIGH;
    if (strcmp(s, "warning") == 0)  return SEV_MEDIUM;
    if (strcmp(s, "style") == 0)    return SEV_LOW;
    if (strcmp(s, "performance") == 0) return SEV_LOW;
    if (strcmp(s, "information") == 0) return SEV_INFO;
    return SEV_UNKNOWN;
}

FindingList *parse_cppcheck(const char *output_file)
{
    FILE *fp = fopen(output_file, "r");
    if (!fp) {
        LOG_ERROR("Cannot open cppcheck output: %s", output_file);
        return NULL;
    }

    FindingList *list = finding_list_new();
    if (!list) { fclose(fp); return NULL; }

    char line[4096];
    Finding *current = NULL;

    while (fgets(line, sizeof(line), fp)) {
        /* Start of an error block */
        if (strstr(line, "<error ")) {
            current = finding_new();
            if (!current) continue;

            current->tool = TOOL_CPPCHECK;

            char *sev = attr_val(line, "severity");
            current->severity = severity_from_cppcheck(sev);
            free(sev);

            current->category = attr_val(line, "id");
            current->message  = attr_val(line, "msg");
        }
        /* First <location> inside an error gives file/line/col */
        else if (strstr(line, "<location ") && current && !current->file) {
            current->file   = attr_val(line, "file");
            char *ln        = attr_val(line, "line");
            char *col       = attr_val(line, "column");
            if (ln)  { current->line   = (int)strtol(ln,  NULL, 10); free(ln); }
            if (col) { current->column = (int)strtol(col, NULL, 10); free(col); }
        }
        /* End of error block — commit finding */
        else if (strstr(line, "</error>") && current) {
            finding_list_add(list, current);
            current = NULL;
        }
    }

    /* Self-closing <error .../> with no </error> */
    if (current)
        finding_list_add(list, current);

    fclose(fp);
    LOG_INFO("cppcheck: parsed %zu findings from %s", list->count, output_file);
    return list;
}
