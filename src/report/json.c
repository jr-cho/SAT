#include "../../include/report/json.h"
#include <stdio.h>
#include <string.h>

static const char *sev_str(Severity s) {
    switch (s) {
        case SEV_INFO:     return "info";
        case SEV_LOW:      return "low";
        case SEV_MEDIUM:   return "medium";
        case SEV_HIGH:     return "high";
        case SEV_CRITICAL: return "critical";
        default:           return "unknown";
    }
}

static const char *tool_str(ToolID t) {
    switch (t) {
        case TOOL_CPPCHECK:   return "cppcheck";
        case TOOL_FLAWFINDER: return "flawfinder";
        case TOOL_GCC:        return "gcc";
        case TOOL_COVERITY:   return "coverity";
        default:              return "unknown";
    }
}

// this just cleans up json so it looks readable
static const char *json_str(const char *s, char *buf, size_t size) {
    if (!s) { buf[0] = '\0'; return buf; }
    size_t i = 0;
    while (*s && i + 2 < size) {
        if      (*s == '"')  { buf[i++] = '\\'; buf[i++] = '"'; }
        else if (*s == '\\') { buf[i++] = '\\'; buf[i++] = '\\'; }
        else if (*s == '\n') { buf[i++] = '\\'; buf[i++] = 'n'; }
        else if (*s == '\r') { buf[i++] = '\\'; buf[i++] = 'r'; }
        else if (*s == '\t') { buf[i++] = '\\'; buf[i++] = 't'; }
        else                 { buf[i++] = *s; }
        s++;
    }
    buf[i] = '\0';
    return buf;
}

int report_write_json(const Database *db, const char *target, const char *out_path) {
    FILE *fp = fopen(out_path, "w");
    if (!fp) return -1;

    char esc[2048];

    fprintf(fp, "{\n");
    fprintf(fp, "  \"target\": \"%s\",\n", json_str(target, esc, sizeof(esc)));
    fprintf(fp, "  \"total\": %zu,\n", db->findings->count);
    fprintf(fp, "  \"findings\": [\n");

    for (size_t i = 0; i < db->findings->count; i++) {
        Finding *f = db->findings->items[i];
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"tool\":     \"%s\",\n",  tool_str(f->tool));
        fprintf(fp, "      \"file\":     \"%s\",\n",  json_str(f->file,     esc, sizeof(esc)));
        fprintf(fp, "      \"line\":     %d,\n",      f->line);
        fprintf(fp, "      \"column\":   %d,\n",      f->column);
        fprintf(fp, "      \"severity\": \"%s\",\n",  sev_str(f->severity));
        fprintf(fp, "      \"category\": \"%s\",\n",  json_str(f->category, esc, sizeof(esc)));
        fprintf(fp, "      \"message\":  \"%s\"\n",   json_str(f->message,  esc, sizeof(esc)));
        fprintf(fp, "    }%s\n", i + 1 < db->findings->count ? "," : "");
    }

    fprintf(fp, "  ]\n}\n");
    fclose(fp);
    return 0;
}
