# Design — Core Functionality and Decisions

This document explains the major design decisions in SAT: why things are structured the way they are, what trade-offs were made, and how the key algorithms work.

---

## Language: C11

SAT is written in C11 rather than C++ or a scripting language for three reasons:

1. **The subject domain.** SAT analyses C source code. Writing the tool in C keeps the dependency chain minimal and means the project can dog food itself — running SAT against its own source is a meaningful test.
2. **No runtime.** There is no JVM, no Python interpreter, no garbage collector. The binary runs anywhere with a libc.
3. **Control.** Memory layout, ownership, and lifetimes are explicit. This matters for a tool whose output is only credible if the tool itself is correct.

The cost is manual memory management and the absence of RAII. The codebase handles this with a strict ownership model: every heap-allocated struct has a paired `_free` function, and callers are responsible for calling it.

---

## Data Model

### `Finding`

The atom of the system. One `Finding` represents one diagnostic from one tool:

```c
typedef struct {
    ToolID   tool;
    char    *file;       // heap-allocated path string
    i32      line;
    i32      column;
    Severity severity;
    char    *category;   // heap-allocated; e.g. CWE-476, buffer
    char    *message;    // heap-allocated diagnostic text
} Finding;
```

`file`, `category`, and `message` are heap-allocated so findings can be stored, filtered, and copied independently of the buffer they were parsed from. `finding_free` frees all three before freeing the struct.

### `FindingList`

A dynamic array of `Finding *` with a capacity-doubling growth strategy. The list owns its items — `finding_list_free` frees each item. Callers that want to keep a subset must deep-copy with `finding_list_filter_*`.

### `Database`

Owns the canonical `FindingList` and the `CorrelationResult`. The database is the single source of truth after ingestion; parsers produce transient lists that are transferred into the database by the `ingest()` helper in `main.c` and `controller.c`.

### Severity enum

```c
typedef enum {
    SEV_UNKNOWN  = 0,
    SEV_INFO     = 1,
    SEV_LOW      = 2,
    SEV_MEDIUM   = 3,
    SEV_HIGH     = 4,
    SEV_CRITICAL = 5
} Severity;
```

The integer values are ordered so that `severity >= min` works as a threshold filter without a lookup table.

---

## Pipeline Architecture

```
argv[1] / m->target
        │
        ▼
  ┌─────────────┐      temp files
  │   Runners   │ ──────────────────► cppcheck_report.xml
  │  (fork/exec)│                     flawfinder_report.txt
  └─────────────┘                     gcc_report.txt
        │                             coverity_report.txt
        ▼
  ┌─────────────┐
  │   Parsers   │  XML / CSV / text → FindingList
  └─────────────┘
        │
        ▼
  ┌─────────────┐
  │  Database   │  ingest → owns all findings
  └─────────────┘
        │
        ▼
  ┌─────────────┐
  │ Correlation │  groups findings within ±3 lines in the same file
  └─────────────┘
        │
        ▼
  ┌─────────────┐
  │   Reports   │  text + JSON output
  └─────────────┘
```

Each stage is independent. The parsers know nothing about the database; the correlator knows nothing about parsers. This makes it straightforward to add a new tool: write a runner, write a parser, call `ingest`.

---

## Correlation Algorithm

The correlator answers: "which findings from different tools are likely about the same defect?"

```c
#define LINE_WINDOW 3

static int same_issue(const Finding *a, const Finding *b) {
    if (!a->file || !b->file) return 0;
    if (strcmp(a->file, b->file) != 0) return 0;
    int diff = a->line - b->line;
    return diff >= -LINE_WINDOW && diff <= LINE_WINDOW;
}
```

Two findings are grouped when they share a filename and their line numbers are within three lines of each other. The three line window accounts for tools reporting slightly different locations for the same issue — one tool may point at the declaration, another at the call site.

The output is a `CorrelatedFinding` array where each element has one slot per tool. Only groups with two or more tools contributing are highlighted in the report.

**Confidence score** — `confidence_score(tool_mask)` returns `popcount(tool_mask) / TOOL_COUNT`. A finding flagged by all four tools gets 1.0 (100%); flagged by one tool gets 0.25 (25%). This is a simple signal, not a calibrated probability.

**Limitation** — the algorithm is O(n × g) where g grows with the number of distinct defects. For large codebases with thousands of findings this would be slow; a sort-then-sweep approach (sort by file + line) would reduce it to O(n log n). The current implementation is sufficient for files up to a few thousand findings.

---

## Severity Normalisation

Each tool uses its own severity vocabulary. `normalize_severity(tool, raw)` maps these to the unified `Severity` enum. The mapping is intentionally conservative: cppcheck `warning` → MEDIUM rather than HIGH, flawfinder level 5 → CRITICAL but level 4 → HIGH. The goal is a ranking that is meaningful when findings from different tools are displayed side by side.

---

## Process Spawning

Tools are run as child processes rather than as shared libraries because:

1. **Isolation.** A crashing tool cannot take down SAT.
2. **No linking.** cppcheck, flawfinder, and Coverity are not C libraries. They are standalone executables with their own build systems and dependencies.
3. **Output capture.** Both `stdout` and `stderr` are redirected to a temp file so parsers can work on a complete, seekable file rather than a live stream.

On POSIX, `fork` + `execvp` is used. On Windows, `CreateProcessA` with an inherited file handle achieves the same result. The argument arrays always include `"--"` before the source file path so that a path beginning with `-` cannot be misinterpreted as a flag.

---

## GUI Threading Model

The GUI runs on the main thread. When the user clicks Run, `controller_run` spawns a single `analysis_thread` via C11 `thrd_create`. The analysis thread:

1. Runs all four tools and parses their output.
2. Builds a new `Database`.
3. Locks `m->lock` (a C11 `mtx_t`), swaps the new database into the model, updates `m->status_msg`, and sets `m->state = GUI_DONE`.
4. Unlocks and exits.

The main thread checks `m->state` under the lock on each frame. When it sees `GUI_DONE` it calls `model_rebuild_visible` and re-renders.

C11 `<threads.h>` was chosen over POSIX `pthread` because it is part of the C11 standard and is available on Windows (MSVC 2022 17.8+, MinGW via GCC) without a separate library. The mutex covers only the state swap — tool execution runs entirely outside the lock so the UI never blocks waiting for a tool.

---

## GUI Library: Raylib

[Raylib](https://www.raylib.com) was chosen for the GUI because:

- **Small footprint.** The library and its dependencies can be compiled from source in seconds.
- **No system GUI toolkit.** No Gtk, no Qt, no Win32 message loop — Raylib abstracts the windowing system completely, which is essential for cross-platform consistency.
- **Submodule friendly.** Raylib is designed to be vendored as a git submodule and built via CMake's `add_subdirectory`, which is exactly how SAT uses it.
- **zlib licence.** Compatible with any project licence.

The trade-off is that Raylib is an immediate-mode renderer, not a widget toolkit. Every button, row, and input field in `ui.c` is drawn manually each frame. This is more code than using a widget library but gives complete control over layout and appearance.

---

## Cross-Platform Strategy

The only platform-specific code lives in two files:

| File | Abstraction |
|------|------------|
| `src/runner/runner.c` | `fork`/`exec`/`waitpid` (POSIX) vs `CreateProcess` (Windows) |
| `src/tool_finder.c` | `access(X_OK)` vs `_access(0)`; `:` vs `;` PATH separator; `strtok_r` vs `strtok_s` |

Everything else is standard C11 plus POSIX file I/O (`fopen`, `fgets`, `fclose`), which Windows supports via the MSVC runtime. The GUI threading uses C11 `<threads.h>` (standard on all target compilers). Raylib provides the platform window and input layer.

CMake selects the correct generator per platform (Ninja / Make on Linux and macOS, MinGW Makefiles or Visual Studio on Windows) and links the correct system libraries (`winmm` on Windows for Raylib's timer).
