#include "../../include/gui/ui.h"
#include "../../include/gui/controller.h"
#include "../../include/core/scoring.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>

/* ── Layout ─────────────────────────────────────────── */
#define TOP_H     55
#define SIDEBAR_W 220
#define DETAIL_H  160
#define ROW_H      44
#define PAD         8

/* ── Palette ─────────────────────────────────────────── */
#define COL_BG       (Color){22,  22,  24,  255}
#define COL_TOPBAR   (Color){28,  28,  32,  255}
#define COL_SIDEBAR  (Color){30,  30,  34,  255}
#define COL_LIST     (Color){22,  22,  24,  255}
#define COL_ROW_A    (Color){26,  26,  28,  255}
#define COL_ROW_B    (Color){30,  30,  32,  255}
#define COL_ROW_SEL  (Color){0,   96,  200, 255}
#define COL_ROW_HOV  (Color){44,  44,  50,  255}
#define COL_DETAIL   (Color){28,  28,  34,  255}
#define COL_BORDER   (Color){55,  55,  62,  255}
#define COL_TEXT     WHITE
#define COL_DIM      (Color){150, 150, 160, 255}
#define COL_INPUT    (Color){38,  38,  44,  255}
#define COL_INPUT_F  (Color){44,  44,  55,  255}
#define COL_BTN_RUN  (Color){30,  160, 75,  255}
#define COL_BTN_RUNH (Color){50,  195, 95,  255}
#define COL_BTN_BUSY (Color){70,  70,  80,  255}
#define COL_MULTI    (Color){100, 60,  200, 255}

static Color sev_color(Severity s)
{
    switch (s) {
        case SEV_CRITICAL: return (Color){200, 30,  30,  255};
        case SEV_HIGH:     return (Color){220, 90,  40,  255};
        case SEV_MEDIUM:   return (Color){210, 160, 0,   255};
        case SEV_LOW:      return (Color){50,  160, 210, 255};
        case SEV_INFO:     return (Color){120, 120, 135, 255};
        default:           return (Color){70,  70,  80,  255};
    }
}

static const char *sev_label(Severity s)
{
    switch (s) {
        case SEV_CRITICAL: return "CRIT";
        case SEV_HIGH:     return "HIGH";
        case SEV_MEDIUM:   return " MED";
        case SEV_LOW:      return " LOW";
        case SEV_INFO:     return "INFO";
        default:           return " ??? ";
    }
}

static const char *tool_label(ToolID t)
{
    switch (t) {
        case TOOL_CPPCHECK:   return "cppcheck";
        case TOOL_FLAWFINDER: return "flawfinder";
        case TOOL_GCC:        return "gcc";
        case TOOL_COVERITY:   return "coverity";
        default:              return "unknown";
    }
}

/* Draw a button; returns true if clicked this frame. */
static bool ui_button(Rectangle r, const char *text, Color bg, Color bg_hov)
{
    Vector2 mouse = GetMousePosition();
    bool hov = CheckCollisionPointRec(mouse, r);
    DrawRectangleRec(r, hov ? bg_hov : bg);
    DrawRectangleLinesEx(r, 1, COL_BORDER);
    int tw = MeasureText(text, 14);
    DrawText(text,
             (int)(r.x + (r.width  - tw) / 2),
             (int)(r.y + (r.height - 14) / 2),
             14, WHITE);
    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

/* Draw a labelled checkbox; returns true if clicked. */
static bool ui_checkbox(int x, int y, bool checked, const char *label, Color on_col)
{
    Rectangle box  = {(float)x, (float)y, 15, 15};
    Rectangle hit  = {(float)x, (float)y - 2, 200, 20};
    Vector2 mouse  = GetMousePosition();
    bool hov       = CheckCollisionPointRec(mouse, hit);

    DrawRectangleRec(box, checked ? on_col : (Color){45, 45, 50, 255});
    DrawRectangleLinesEx(box, 1, COL_BORDER);
    if (checked) DrawText("x", x + 3, y + 1, 12, WHITE);
    DrawText(label, x + 22, y, 13, hov ? COL_TEXT : COL_DIM);

    return hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

/* Draw text truncated to max_w pixels (appends "..."). */
static void ui_text_clip(const char *text, int x, int y,
                          int font_size, Color col, int max_w)
{
    if (!text || !*text) return;
    char buf[512];
    int  len = (int)strlen(text);
    if (len >= (int)sizeof(buf)) len = (int)sizeof(buf) - 1;
    memcpy(buf, text, (size_t)len);
    buf[len] = '\0';

    while (len > 3 && MeasureText(buf, font_size) > max_w) {
        buf[--len] = '\0';
        if (len >= 3) { buf[len-1] = '.'; buf[len-2] = '.'; buf[len-3] = '.'; }
    }
    DrawText(buf, x, y, font_size, col);
}

/* ── Top bar ─────────────────────────────────────────── */
static void draw_topbar(GUIModel *m, int w)
{
    DrawRectangle(0, 0, w, TOP_H, COL_TOPBAR);
    DrawLine(0, TOP_H - 1, w, TOP_H - 1, COL_BORDER);

    DrawText("SAT", PAD + 2, 17, 22, (Color){0, 140, 255, 255});

    /* File path input */
    Rectangle inp = {68, 12, (float)(w - 68 - 145 - 16), 30};
    Vector2 mouse = GetMousePosition();
    bool hov_inp  = CheckCollisionPointRec(mouse, inp);

    DrawRectangleRec(inp, m->input_focused ? COL_INPUT_F : COL_INPUT);
    DrawRectangleLinesEx(inp, 1,
        m->input_focused ? (Color){0, 120, 220, 255} : COL_BORDER);

    /* Show path (right-aligned when long) */
    const char *display  = m->target[0] ? m->target : "path/to/source.c";
    Color       text_col = m->target[0] ? COL_TEXT  : COL_DIM;
    int         avail    = (int)inp.width - 10;
    int         tw       = MeasureText(display, 13);
    if (tw <= avail) {
        DrawText(display, (int)inp.x + 5, (int)inp.y + 8, 13, text_col);
    } else {
        /* Show only the rightmost portion that fits */
        const char *p = display + strlen(display);
        while (p > display && MeasureText(p, 13) < avail - MeasureText("...", 13)) p--;
        char clip[512];
        snprintf(clip, sizeof(clip), "...%s", p);
        DrawText(clip, (int)inp.x + 5, (int)inp.y + 8, 13, text_col);
    }

    /* Blinking cursor */
    if (m->input_focused && ((int)(GetTime() * 2) & 1)) {
        int cx = (int)inp.x + 5 + MeasureText(m->target, 13);
        if (cx > (int)(inp.x + inp.width - 6))
            cx = (int)(inp.x + inp.width - 6);
        DrawLine(cx, (int)inp.y + 5, cx, (int)inp.y + 24, WHITE);
    }

    if (hov_inp  && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) m->input_focused = true;
    if (!hov_inp && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) m->input_focused = false;

    /* Status text (left of Run button) */
    int         sm_w = MeasureText(m->status_msg, 11);
    Rectangle   run  = {(float)(w - 140), 12, 130, 30};
    int         sm_x = (int)run.x - sm_w - 10;
    if (sm_x > (int)(inp.x + inp.width + 6))
        DrawText(m->status_msg, sm_x, 21, 11, COL_DIM);

    /* Run button */
    bool busy = (m->state == GUI_RUNNING);
    if (ui_button(run,
                  busy ? "Running..." : "Run Analysis",
                  busy ? COL_BTN_BUSY : COL_BTN_RUN,
                  busy ? COL_BTN_BUSY : COL_BTN_RUNH)
        && !busy)
    {
        controller_run(m);
    }
}

/* ── Sidebar ─────────────────────────────────────────── */
static void draw_sidebar(GUIModel *m, int h)
{
    DrawRectangle(0, TOP_H, SIDEBAR_W, h - TOP_H, COL_SIDEBAR);
    DrawLine(SIDEBAR_W - 1, TOP_H, SIDEBAR_W - 1, h, COL_BORDER);

    int y = TOP_H + PAD + 4;

    /* ── Tools ── */
    DrawText("TOOLS", PAD, y, 11, COL_DIM);
    y += 18;

    static const char *tnames[] = {"cppcheck", "flawfinder", "gcc", "coverity"};
    static Color tcolors[] = {
        {80,  160, 220, 255},
        {200, 140, 50,  255},
        {100, 200, 100, 255},
        {200, 80,  80,  255}
    };
    for (int t = 0; t < TOOL_COUNT; t++) {
        if (ui_checkbox(PAD, y, m->tool_enabled[t], tnames[t], tcolors[t]))
            controller_toggle_tool(m, t);
        y += 22;
    }

    y += 8;
    DrawLine(PAD, y, SIDEBAR_W - PAD, y, COL_BORDER);
    y += 10;

    /* ── Min Severity ── */
    DrawText("MIN SEVERITY", PAD, y, 11, COL_DIM);
    y += 18;

    static const Severity  slevels[] = {SEV_UNKNOWN, SEV_INFO, SEV_LOW,
                                         SEV_MEDIUM,  SEV_HIGH, SEV_CRITICAL};
    static const char     *snames[]  = {"All", "Info", "Low",
                                         "Medium", "High", "Critical"};
    Vector2 mouse = GetMousePosition();
    for (int i = 0; i < 6; i++) {
        bool    active = (m->min_severity == slevels[i]);
        Rectangle r    = {(float)PAD, (float)y, (float)(SIDEBAR_W - PAD * 2), 22};
        bool    hov    = CheckCollisionPointRec(mouse, r);

        if (active)     DrawRectangleRec(r, (Color){0,  80, 160, 255});
        else if (hov)   DrawRectangleRec(r, (Color){50, 50,  58, 255});

        Color dot = (i == 0) ? COL_DIM : sev_color(slevels[i]);
        DrawRectangle((int)r.x + 2, y + 6, 10, 10, dot);
        DrawText(snames[i], (int)r.x + 18, y + 4, 13,
                 active ? COL_TEXT : COL_DIM);

        if (hov && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            controller_set_min_severity(m, slevels[i]);
        y += 24;
    }

    y += 8;
    DrawLine(PAD, y, SIDEBAR_W - PAD, y, COL_BORDER);
    y += 10;

    /* Finding count */
    char buf[64];
    snprintf(buf, sizeof(buf), "%d finding%s shown",
             m->visible_count, m->visible_count == 1 ? "" : "s");
    DrawText(buf, PAD, y, 12, COL_DIM);
}

/* ── Finding list ─────────────────────────────────────── */
static void draw_finding_list(GUIModel *m, int x, int y, int w, int h)
{
    DrawRectangle(x, y, w, h, COL_LIST);

    if (m->visible_count == 0) {
        const char *msg =
            (m->state == GUI_RUNNING) ? "Running analysis..." :
            (m->state == GUI_IDLE)    ? "No analysis run yet — enter a file and click Run Analysis" :
                                        "No findings match the current filters";
        int tw = MeasureText(msg, 15);
        DrawText(msg, x + (w - tw) / 2, y + h / 2 - 8, 15, COL_DIM);
        return;
    }

    Vector2 mouse = GetMousePosition();
    int     row_y = y;
    int     end   = m->scroll + m->rows_visible;
    if (end > m->visible_count) end = m->visible_count;

    for (int i = m->scroll; i < end; i++) {
        Finding *f    = model_get_visible(m, i);
        if (!f) continue;

        bool selected = (i == m->selected);
        bool hovered  = CheckCollisionPointRec(
            mouse, (Rectangle){(float)x, (float)row_y, (float)w, ROW_H});

        Color row_bg = selected ? COL_ROW_SEL :
                       hovered  ? COL_ROW_HOV :
                       (i & 1)  ? COL_ROW_B   : COL_ROW_A;
        DrawRectangle(x, row_y, w, ROW_H, row_bg);
        DrawLine(x, row_y + ROW_H - 1, x + w, row_y + ROW_H - 1, COL_BORDER);

        /* Severity badge */
        Color sc = sev_color(f->severity);
        DrawRectangle(x + 6, row_y + 9, 44, 18, sc);
        DrawText(sev_label(f->severity), x + 8, row_y + 12, 12, WHITE);

        /* File:line / tool */
        char loc[256];
        snprintf(loc, sizeof(loc), "%s:%d",
                 f->file ? f->file : "?", f->line);
        ui_text_clip(loc, x + 60, row_y + 7,  13,
                     selected ? WHITE : COL_TEXT, 280);
        DrawText(tool_label(f->tool), x + 60, row_y + 24, 11,
                 selected ? (Color){190, 210, 255, 255} : COL_DIM);

        /* Message */
        ui_text_clip(f->message ? f->message : "(no message)",
                     x + 350, row_y + 15, 13,
                     selected ? WHITE : (Color){185, 185, 200, 255},
                     w - 350 - 50);

        /* Multi-tool badge */
        if (m->db && m->db->correlation) {
            for (u32 ci = 0; ci < m->db->correlation->count; ci++) {
                CorrelatedFinding *cf = m->db->correlation->items[ci];
                bool found = false;
                for (int t = 0; t < TOOL_COUNT; t++)
                    if (cf->sources[t] == f) { found = true; break; }
                if (!found) continue;
                u32 mask = cf->tool_mask, tc = 0;
                while (mask) { tc += mask & 1u; mask >>= 1; }
                if (tc < 2) break;
                char badge[8];
                snprintf(badge, sizeof(badge), "%ux", tc);
                int bw = MeasureText(badge, 11) + 8;
                DrawRectangle(x + w - bw - 10, row_y + 12, bw, 18, COL_MULTI);
                DrawText(badge, x + w - bw - 7, row_y + 14, 11, WHITE);
                break;
            }
        }

        if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            controller_select(m, i);

        row_y += ROW_H;
    }

    /* Scrollbar */
    if (m->visible_count > m->rows_visible) {
        int   sb_x    = x + w - 7;
        float ratio   = (float)m->rows_visible / (float)m->visible_count;
        float t_h     = ratio * (float)h;
        float t_y     = (float)y + (float)m->scroll / (float)m->visible_count * (float)h;
        DrawRectangle(sb_x, y, 7, h, (Color){38, 38, 42, 255});
        DrawRectangle(sb_x + 1, (int)t_y, 5, (int)t_h, COL_DIM);
    }
}

/* ── Detail panel ─────────────────────────────────────── */
static void draw_detail(GUIModel *m, int y, int w, int h)
{
    DrawRectangle(0, y, w, h, COL_DETAIL);
    DrawLine(0, y, w, y, COL_BORDER);

    Finding *f = model_get_selected(m);
    if (!f) {
        int tw = MeasureText("Select a finding to see details", 14);
        DrawText("Select a finding to see details",
                 SIDEBAR_W + (w - SIDEBAR_W - tw) / 2,
                 y + h / 2 - 7, 14, COL_DIM);
        return;
    }

    int lx = SIDEBAR_W + PAD * 2;
    int ly = y + PAD;

    /* Severity badge */
    Color sc = sev_color(f->severity);
    DrawRectangle(lx, ly, 50, 22, sc);
    DrawText(sev_label(f->severity), lx + 4, ly + 4, 13, WHITE);
    lx += 60;

    /* Location */
    char loc[512];
    snprintf(loc, sizeof(loc), "%s   line %d   col %d",
             f->file ? f->file : "?", f->line, f->column);
    DrawText(loc, lx, ly + 3, 14, COL_TEXT);

    /* Tool + category */
    char meta[512];
    snprintf(meta, sizeof(meta), "Tool: %s     Category: %s",
             tool_label(f->tool),
             f->category ? f->category : "(none)");
    DrawText(meta, lx, ly + 22, 12, COL_DIM);

    /* Message */
    int mlx = SIDEBAR_W + PAD * 2;
    DrawText("Message:", mlx, ly + 50, 11, COL_DIM);
    ui_text_clip(f->message ? f->message : "(none)",
                 mlx + 74, ly + 48, 13, COL_TEXT, w - mlx - 80);

    /* Correlation row */
    if (m->db && m->db->correlation) {
        for (u32 ci = 0; ci < m->db->correlation->count; ci++) {
            CorrelatedFinding *cf = m->db->correlation->items[ci];
            bool found = false;
            for (int t = 0; t < TOOL_COUNT; t++)
                if (cf->sources[t] == f) { found = true; break; }
            if (!found) continue;

            u32 mask = cf->tool_mask, tc = 0;
            while (mask) { tc += mask & 1u; mask >>= 1; }
            if (tc < 2) break;

            int tx = mlx, ty = ly + 76;
            DrawText("Also found by:", tx, ty, 11, COL_DIM);
            tx += MeasureText("Also found by:", 11) + 8;

            for (int t = 0; t < TOOL_COUNT; t++) {
                if (!cf->sources[t] || cf->sources[t] == f) continue;
                const char *tl = tool_label(t);
                int bw = MeasureText(tl, 11) + 10;
                DrawRectangle(tx, ty - 2, bw, 16, (Color){55, 90, 170, 255});
                DrawText(tl, tx + 5, ty, 11, WHITE);
                tx += bw + 6;
            }

            float conf = confidence_score(cf->tool_mask);
            char  cbuf[32];
            snprintf(cbuf, sizeof(cbuf), "  Confidence: %.0f%%", conf * 100.0f);
            DrawText(cbuf, tx, ty, 11, (Color){90, 200, 100, 255});
            break;
        }
    }
}

/* ── Public API ──────────────────────────────────────── */
void ui_init(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(WIN_W, WIN_H, "SAT \xe2\x80\x94 Static Analysis Tool");
    SetTargetFPS(60);
}

bool ui_frame(GUIModel *m)
{
    if (WindowShouldClose()) return false;

    int w = GetScreenWidth();
    int h = GetScreenHeight();
    m->rows_visible = (h - TOP_H - DETAIL_H) / ROW_H;
    if (m->rows_visible < 1) m->rows_visible = 1;

    /* ── Keyboard input for file path ── */
    if (m->input_focused) {
        int c;
        while ((c = GetCharPressed()) != 0) {
            int len = (int)strlen(m->target);
            if (c >= 32 && len < (int)sizeof(m->target) - 2) {
                m->target[len]     = (char)c;
                m->target[len + 1] = '\0';
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            int len = (int)strlen(m->target);
            if (len > 0) m->target[len - 1] = '\0';
        }
        if (IsKeyPressed(KEY_ENTER)) {
            m->input_focused = false;
            if (m->state != GUI_RUNNING) controller_run(m);
        }
    }

    /* ── Scroll wheel on list area ── */
    Vector2 mouse = GetMousePosition();
    if (mouse.x > SIDEBAR_W && mouse.y > TOP_H && mouse.y < h - DETAIL_H) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0) controller_scroll(m, -(int)wheel);
    }

    /* ── Arrow keys for list navigation ── */
    if (!m->input_focused && m->visible_count > 0) {
        if (IsKeyPressed(KEY_DOWN)) {
            int nxt = (m->selected < 0) ? 0 : m->selected + 1;
            controller_select(m, nxt);
            if (m->selected >= m->scroll + m->rows_visible)
                controller_scroll(m, 1);
        }
        if (IsKeyPressed(KEY_UP) && m->selected > 0) {
            controller_select(m, m->selected - 1);
            if (m->selected < m->scroll)
                controller_scroll(m, -1);
        }
    }

    /* ── Detect analysis completion ── */
    static bool was_running = false;
    mtx_lock(&m->lock);
    GUIState st = m->state;
    mtx_unlock(&m->lock);

    if (was_running && st == GUI_DONE) {
        model_rebuild_visible(m);
        was_running = false;
    }
    if (st == GUI_RUNNING) was_running = true;

    /* ── Draw ── */
    BeginDrawing();
    ClearBackground(COL_BG);
    draw_topbar(m, w);
    draw_sidebar(m, h);
    draw_finding_list(m, SIDEBAR_W, TOP_H, w - SIDEBAR_W, h - TOP_H - DETAIL_H);
    draw_detail(m, h - DETAIL_H, w, DETAIL_H);
    EndDrawing();

    return true;
}

void ui_shutdown(void)
{
    CloseWindow();
}
