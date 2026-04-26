# Architecture — File Reference

SAT is a C11 command-line and GUI tool that runs four static analysis tools against a C source file, parses their output into a unified data model, correlates overlapping findings, and renders a report. The codebase is split into seven subsystems.

```
src/
├── main.c                    CLI entry point
├── tool_finder.c             GCC version discovery
├── common/                   Shared utilities and logging
├── runner/                   Process spawning per tool
├── parser/                   Tool output → Finding structs
├── core/                     Database, correlation, scoring
├── report/                   Text and JSON output
└── gui/                      Raylib GUI
include/                      Public headers (mirrors src/ layout)
```

---

## Entry Points

### `src/main.c`

The CLI entry point. Accepts a single source file path as `argv[1]`.

Pipeline in order:
1. Call each runner to execute the analysis tool and write its output to a temp file.
2. Call the matching parser to read that file and return a `FindingList`.
3. Ingest each `FindingList` into the central `Database`.
4. Run `db_correlate()` to group related findings.
5. Print the report to `stdout` and write `analysis_report.txt` / `analysis_report.json`.
6. Delete the temp files.

### `src/gui/main.c`

The GUI entry point. Initialises a `GUIModel`, then drives the Raylib frame loop by calling `ui_frame()` until the window closes.

---

## Tool Discovery

### `src/tool_finder.c` / `include/tool_finder.h`

`find_gcc_analyzer()` returns the path to the first usable GCC on the system. It checks the `GCC_ANALYZER` environment variable first, then walks a priority list of versioned names (`gcc-15`, `gcc-14`, `gcc-13`, `gcc`) against the system `PATH`.

Absolute Homebrew paths (`/opt/homebrew/bin/gcc-*`) and `/usr/local/bin/gcc-*` are included as fallback candidates for macOS where `gcc` in `PATH` is often Apple Clang.

Platform notes:
- Uses `strtok_r` (POSIX) / `strtok_s` (MSVC) for thread-safe PATH splitting.
- Uses `_access` (Windows) / `access(X_OK)` (POSIX) to check executability.
- PATH separator is `;` on Windows, `:` everywhere else.

---

## Common Utilities

### `src/common/utils.c` / `include/common/utils.h`

| Function | Purpose |
|----------|---------|
| `str_dup(s)` | `malloc` + `memcpy` clone of a string; returns NULL if `s` is NULL |
| `str_trim(s)` | In-place leading/trailing whitespace removal; returns the trimmed pointer |
| `str_starts_with(s, prefix)` | NULL-safe prefix test via `strncmp` |
| `finding_new()` | `calloc` a zeroed `Finding`; sets `line` and `column` to `-1` |
| `finding_free(f)` | Frees all heap members of a `Finding` then frees the struct itself |

### `src/common/logging.c` / `include/common/logging.h`

A thin wrapper around `fprintf`. Four macros (`LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, `LOG_ERROR`) inject `__FILE__` and `__LINE__` automatically. `LOG_WARN` and `LOG_ERROR` write to `stderr`; the rest go to `stdout`. The active level is set at runtime with `log_set_level()`.

### `include/common/types.h`

Defines the two enums and the core struct used everywhere:

- `ToolID` — `TOOL_CPPCHECK`, `TOOL_FLAWFINDER`, `TOOL_GCC`, `TOOL_COVERITY`, `TOOL_COUNT`
- `Severity` — `SEV_UNKNOWN` through `SEV_CRITICAL` (ordered low-to-high so `>=` comparisons work)
- `Finding` — one diagnostic: `tool`, `file` (heap string), `line`, `column`, `severity`, `category` (heap string), `message` (heap string)

Integer typedefs (`i8`/`u8` … `i64`/`u64`) are also here.

---

## Runner Layer

Each runner constructs an argument array and calls `run_tool()`. The `--` separator is placed before `source_file` in every array so a path beginning with `-` cannot be interpreted as a flag (argument injection defence).

### `src/runner/runner.c` / `include/runner/runner.h`

`run_tool(tool, args[], output_file)` — spawns the tool, redirects both `stdout` and `stderr` to `output_file`, waits for the process, and returns its exit code.

- **POSIX**: `fork()` → `open()` → `dup2()` → `execvp()` → `waitpid()`. Exit code is extracted only when `WIFEXITED()` is true; otherwise returns `-1`.
- **Windows**: builds a quoted command-line string, opens the output file with inheritable handle, calls `CreateProcessA()`, waits with `WaitForSingleObject()`, retrieves the code with `GetExitCodeProcess()`.

### `src/runner/cppcheck.c`

Invokes: `cppcheck --xml --xml-version=2 --enable=all --suppress=missingIncludeSystem -- <file>`

### `src/runner/flawfinder.c`

Invokes: `flawfinder --dataonly --csv -- <file>`

### `src/runner/gcc.c`

Invokes: `<gcc> -fanalyzer -o /dev/null -- <file>`

Calls `find_gcc_analyzer()` to resolve the GCC path first; returns `-1` immediately if none is found.

### `src/runner/coverity.c`

Invokes: `cov-analyze --dir cov-int -- <file>`

Coverity requires a prior `cov-build` capture step. This runner emits a `LOG_WARN` and proceeds; callers check the return code.

---

## Parser Layer

### `src/parser/parser.c` / `include/parser/parser.h`

Manages `FindingList` — a heap-allocated dynamic array of `Finding *`.

| Function | Purpose |
|----------|---------|
| `finding_list_new()` | Allocates list with initial capacity 16 |
| `finding_list_add(list, f)` | Appends `f`; doubles capacity via `realloc` when full |
| `finding_list_free(list)` | Calls `finding_free` on every item, then frees the array and struct |

### `src/parser/cppcheck_parser.c`

Parses cppcheck's XML v2 output line-by-line. Looks for `<error …>` tags to open a finding, `<location …>` to set file/line/column, and `</error>` to commit it. Attribute values are extracted by `attr_val()`, which finds `attr="` and copies up to the closing `"`.

### `src/parser/flawfinder_parser.c`

Parses flawfinder's CSV output. A local `csv_next_field()` handles RFC-4180 double-quote escaping (`""` → `"`). Column layout expected:

| Index | Field |
|-------|-------|
| 0 | File |
| 1 | Line |
| 2 | Column |
| 4 | Risk level (0–5) |
| 5 | Category |
| 6 | Name |
| 7 | Warning text |
| 10 | CWEs (optional) |

### `src/parser/gcc_parser.c`

Parses GCC's diagnostic line format: `<file>:<line>:<col>: <type>: <message> [CWE-NNN] [-Wflag]`

`parse_diag_line()` walks the line character by character to find the first colon followed by a digit (the line number), then uses `strtol` for the numeric fields. `extract_cwe()` pulls `[CWE-NNN]` from the message and trims both the CWE annotation and any trailing `[-Wflag]`.

Lines that do not match the pattern (code context, event traces, arrows) are silently skipped.

### `src/parser/coverity_parser.c`

Parses Coverity's text report format with a two-state machine (`COV_IDLE` / `COV_IN_FINDING`):

1. In `COV_IDLE`: look for a line containing ` CID `. Extract file, line, and checker name. Transition to `COV_IN_FINDING`.
2. In `COV_IN_FINDING`: look for ` high: `, ` medium: `, or ` low: `. Extract the message after the event label. Commit the finding. A blank line without a severity line discards the pending finding.

---

## Core Layer

### `src/core/database.c` / `include/core/database.h`

The `Database` struct owns the canonical `FindingList` and the `CorrelationResult`. Key functions:

| Function | Purpose |
|----------|---------|
| `db_new()` | Allocates and zero-initialises a `Database` with an empty `FindingList` |
| `db_add_finding(db, f)` | Appends a finding; the database takes ownership |
| `db_correlate(db)` | Runs the correlator and stores the result; frees any previous result |
| `db_query(db, tool, min_sev)` | Returns a new `FindingList` with deep-copied findings matching the filters |
| `db_free(db)` | Frees findings, correlation result, file list, and the struct |

### `src/core/findings.c` / `include/core/findings.h`

Filter operations that produce new `FindingList` instances with deep-copied findings. Used by the GUI to produce filtered views without mutating the database.

| Function | Purpose |
|----------|---------|
| `finding_list_filter_tool(list, tool)` | Keeps only findings from the given tool |
| `finding_list_filter_severity(list, min)` | Keeps findings with `severity >= min` |
| `finding_list_get(list, index)` | Bounds-checked index access |

### `src/core/correlation.c` / `include/core/correlation.h`

Groups findings that likely refer to the same defect. Two findings are considered the same issue when they are in the same file and their line numbers are within `LINE_WINDOW` (3) of each other.

The result is a `CorrelationResult` — a dynamic array of `CorrelatedFinding`. Each `CorrelatedFinding` holds one `Finding *` slot per tool (indexed by `ToolID`), a bitmask (`tool_mask`) of which tools fired, and a `consensus_severity` (the maximum severity seen across contributing findings).

The algorithm is O(n × g) where n = number of findings and g = number of groups. For typical codebases this is fast enough; it would need a sort-then-sweep optimisation for very large finding sets.

### `src/core/scoring.c` / `include/core/scoring.h`

| Function | Purpose |
|----------|---------|
| `normalize_severity(tool, raw)` | Maps a tool-specific severity string to the unified `Severity` enum |
| `confidence_score(tool_mask)` | Returns `(number of tools that fired) / TOOL_COUNT` as a float in `[0, 1]` |

`normalize_severity` is used by parsers that receive raw strings rather than enumerated values.

---

## Report Layer

### `src/report/report.c` / `include/report/report.h`

`report_print(db, target, out)` writes a human-readable report to any `FILE *`:

1. Header with target path and timestamp.
2. Summary table — one row per tool showing finding counts by severity.
3. Correlated findings section — only groups detected by two or more tools, with confidence percentage.
4. Full findings — one section per tool with file, line, column, severity badge, category, and message.

`report_write(db, target, path)` opens a file and calls `report_print` into it.

### `src/report/json.c` / `include/report/json.h`

`report_write_json(db, target, path)` writes a JSON file:

```json
{
  "target": "...",
  "total": 42,
  "findings": [
    { "tool": "cppcheck", "file": "...", "line": 10, "column": 5,
      "severity": "high", "category": "...", "message": "..." }
  ]
}
```

String values are escaped by `json_str()`, which handles `"`, `\`, `\n`, `\r`, and `\t`. Output truncates silently to 2047 characters per field — sufficient for any normal diagnostic message.

---

## GUI Layer

The GUI is built with [Raylib](https://www.raylib.com) and is only compiled when the `external/raylib` submodule is present. Threading uses C11 `<threads.h>` for portability across Linux, macOS, and Windows.

### `include/gui/model.h` / `src/gui/model.c`

`GUIModel` is the single shared state object. It contains:
- A `Database *` with all findings.
- A `char target[512]` file path.
- A `thrd_t worker` and `mtx_t lock` for the analysis thread.
- Filter state: `tool_enabled[TOOL_COUNT]` booleans and `min_severity`.
- View state: `visible_idx[]` (indices of findings that pass the current filters), `scroll`, `selected`, `rows_visible`.

`model_rebuild_visible()` rebuilds `visible_idx` by walking all findings and keeping those that pass the active tool and severity filters. Called whenever filters change or new findings arrive.

### `src/gui/controller.c` / `include/gui/controller.h`

Event handlers called by `ui_frame()`:

| Function | Triggered by |
|----------|-------------|
| `controller_run(m)` | Run button / Enter key |
| `controller_toggle_tool(m, idx)` | Tool checkbox click |
| `controller_set_min_severity(m, sev)` | Severity filter click |
| `controller_select(m, idx)` | Row click |
| `controller_scroll(m, delta)` | Mouse wheel / arrow keys |

`controller_run` spawns `analysis_thread` via `thrd_create`. The thread runs the full pipeline (all four runners → parsers → ingest → correlate) while the UI remains responsive, then swaps the new `Database` into the model under the mutex.

### `src/gui/ui.c` / `include/gui/ui.h`

`ui_frame(m)` is called once per Raylib frame (60 fps target). It:
1. Processes keyboard input for the file path field and arrow-key navigation.
2. Checks whether the analysis thread completed and calls `model_rebuild_visible` if so.
3. Draws four regions: top bar (path input + Run button + status), sidebar (tool and severity filters), finding list, detail panel.

Layout is pixel-based using constants (`TOP_H`, `SIDEBAR_W`, `DETAIL_H`, `ROW_H`). The window is resizable; `rows_visible` is recalculated each frame from the current window height.

---

## Build System

### `CMakeLists.txt`

The primary cross-platform build file. Produces `analyzer` (CLI) unconditionally and `sat-gui` (GUI) when `external/raylib/CMakeLists.txt` exists. Sanitizer and static-analysis builds are opt-in via `-DENABLE_ASAN=ON`, `-DENABLE_TSAN=ON`, `-DENABLE_ANALYZE=ON`.

See [install.md](install.md) for usage.

### `Makefile`

A convenience file for Linux / macOS CLI-only (`analyzer`) builds. Not required — CMake handles everything — but faster for iterating on the non-GUI code. Includes `asan`, `tsan`, `analyze`, and `scan` targets.

### `install.sh` / `install.ps1`

Interactive install scripts that detect the available toolchain, optionally fetch the Raylib submodule, and drive CMake. See [install.md](install.md).
