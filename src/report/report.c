#include "../../include/report/report.h"
#include "../../include/core/scoring.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *sev_name(Severity s) {
    switch (s) {
        case SEV_INFO:     return "INFO";
        case SEV_LOW:      return "LOW";
        case SEV_MEDIUM:   return "MEDIUM";
        case SEV_HIGH:     return "HIGH";
        case SEV_CRITICAL: return "CRITICAL";
        default:           return "UNKNOWN";
    }
}

static const char *tool_name(ToolID t) {
    switch (t) {
        case TOOL_CPPCHECK:   return "cppcheck";
        case TOOL_FLAWFINDER: return "flawfinder";
        case TOOL_GCC:        return "gcc (-fanalyzer)";
        case TOOL_COVERITY:   return "coverity";
        default:              return "unknown";
    }
}

static void print_divider(FILE *out, char c, int width) {
    for (int i = 0; i < width; i++) fputc(c, out);
    fputc('\n', out);
}

static void print_summary(const Database *db, FILE *out) {
    u32 counts[TOOL_COUNT][6] = {{0}};
    u32 tool_total[TOOL_COUNT] = {0};

    for (size_t i = 0; i < db->findings->count; i++) {
        Finding *f = db->findings->items[i];
        counts[f->tool][f->severity]++;
        tool_total[f->tool]++;
    }

    fprintf(out, "\nSUMMARY\n");
    print_divider(out, '-', 70);
    fprintf(out, "%-20s %8s %6s %6s %8s %6s %10s\n",
            "Tool", "Total", "Info", "Low", "Medium", "High", "Critical");
    print_divider(out, '-', 70);

    for (int t = 0; t < TOOL_COUNT; t++) {
        if (tool_total[t] == 0) {
            fprintf(out, "%-20s %8s\n", tool_name(t), "(not run)");
        } else {
            fprintf(out, "%-20s %8u %6u %6u %8u %6u %10u\n",
                    tool_name(t), tool_total[t],
                    counts[t][SEV_INFO],   counts[t][SEV_LOW],
                    counts[t][SEV_MEDIUM], counts[t][SEV_HIGH],
                    counts[t][SEV_CRITICAL]);
        }
    }

    print_divider(out, '-', 70);

    u32 grand = 0, totals[6] = {0};
    for (int t = 0; t < TOOL_COUNT; t++) {
        grand += tool_total[t];
        for (int s = 0; s < 6; s++) totals[s] += counts[t][s];
    }
    fprintf(out, "%-20s %8u %6u %6u %8u %6u %10u\n",
            "TOTAL", grand,
            totals[SEV_INFO], totals[SEV_LOW],
            totals[SEV_MEDIUM], totals[SEV_HIGH], totals[SEV_CRITICAL]);
}

static void print_correlated(const Database *db, FILE *out) {
    if (!db->correlation) return;

    u32 multi = 0;
    for (u32 i = 0; i < db->correlation->count; i++) {
        u32 mask = db->correlation->items[i]->tool_mask;
        u32 bits = mask - ((mask >> 1) & 0x55555555u);
        bits = (bits & 0x33333333u) + ((bits >> 2) & 0x33333333u);
        bits = (bits + (bits >> 4)) & 0x0f0f0f0fu;
        if ((bits * 0x01010101u) >> 24 > 1) multi++;
    }
    if (multi == 0) return;

    fprintf(out, "\nCORRELATED FINDINGS  "
                 "(same issue flagged by multiple tools)\n");
    print_divider(out, '-', 70);

    for (u32 i = 0; i < db->correlation->count; i++) {
        CorrelatedFinding *cf = db->correlation->items[i];
        float conf = confidence_score(cf->tool_mask);

        // pop counter
        u32 mask = cf->tool_mask;
        u32 bits = mask - ((mask >> 1) & 0x55555555u);
        bits = (bits & 0x33333333u) + ((bits >> 2) & 0x33333333u);
        bits = (bits + (bits >> 4)) & 0x0f0f0f0fu;
        if ((bits * 0x01010101u) >> 24 < 2) continue;

        Finding *anchor = NULL;
        for (int t = 0; t < TOOL_COUNT; t++)
            if (cf->sources[t]) { anchor = cf->sources[t]; break; }

        fprintf(out, "[%s] %s:%d  —  confidence %.0f%%  —  detected by:",
                sev_name(cf->consensus_severity),
                anchor && anchor->file ? anchor->file : "?",
                anchor ? anchor->line : -1,
                conf * 100.0f);

        for (int t = 0; t < TOOL_COUNT; t++)
            if (cf->sources[t]) fprintf(out, " %s", tool_name(t));
        fputc('\n', out);

        for (int t = 0; t < TOOL_COUNT; t++) {
            Finding *f = cf->sources[t];
            if (!f) continue;
            fprintf(out, "  %-18s %s\n",
                    tool_name(t),
                    f->message ? f->message : "(no message)");
        }
        fputc('\n', out);
    }
}

static void print_tool_section(const Database *db, ToolID tool, FILE *out) {
    fprintf(out, "\n[%s]\n", tool_name(tool));
    int printed = 0;

    for (size_t i = 0; i < db->findings->count; i++) {
        Finding *f = db->findings->items[i];
        if (f->tool != tool) continue;

        fprintf(out, "  [%-8s]  %s:%d",
                sev_name(f->severity),
                f->file ? f->file : "?",
                f->line);
        if (f->column >= 0)
            fprintf(out, ":%d", f->column);
        if (f->category)
            fprintf(out, "  (%s)", f->category);
        fprintf(out, "\n             %s\n",
                f->message ? f->message : "(no message)");
        printed++;
    }

    if (!printed)
        fprintf(out, "  (no findings)\n");
}

void report_print(const Database *db, const char *target, FILE *out) {
    print_divider(out, '=', 70);
    fprintf(out, "  Static Analysis Report\n");
    fprintf(out, "  Target : %s\n", target ? target : "(unknown)");

    time_t now = time(NULL);
    char   date[32];
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(out, "  Date   : %s\n", date);
    print_divider(out, '=', 70);

    print_summary(db, out);
    print_correlated(db, out);

    fprintf(out, "\nALL FINDINGS\n");
    print_divider(out, '-', 70);
    for (int t = 0; t < TOOL_COUNT; t++)
        print_tool_section(db, (ToolID)t, out);

    fputc('\n', out);
    print_divider(out, '=', 70);
}

int report_write(const Database *db, const char *target, const char *out_path) {
    FILE *fp = fopen(out_path, "w");
    if (!fp) return -1;
    report_print(db, target, fp);
    fclose(fp);
    return 0;
}
