# Security Hardening

This document records the security fixes applied during the April 2026 audit.

---

## Argument Injection — Runner Layer

**Files:** `src/runner/cppcheck.c`, `src/runner/flawfinder.c`, `src/runner/gcc.c`, `src/runner/coverity.c`

The user-supplied target path (`argv[1]` / `m->target`) was passed directly as the last positional argument to each tool's `execvp` call. A path beginning with `--` (e.g. `--output=evil`) would be interpreted as a flag by the tool rather than a filename.

**Fix:** Added `"--"` before `source_file` in every tool's argument array. The `--` POSIX convention signals end-of-options; all four tools honour it.

---

## `WIFEXITED` Guard in Process Runner

**File:** `src/runner/runner.c`

`WEXITSTATUS(status)` was called unconditionally after `waitpid`. POSIX only guarantees its value is meaningful when `WIFEXITED(status)` is true. If a child process was killed by a signal the macro extracts garbage bits from the status word, which is undefined behaviour.

**Fix:** Exit code is now `WIFEXITED(status) ? WEXITSTATUS(status) : -1`.

---

## Thread-Safety — `strtok` → `strtok_r`

**File:** `src/tool_finder.c`

`strtok` stores its position in a hidden static buffer shared across all threads. `find_gcc_analyzer()` is called from the GUI's `analysis_thread` (a `pthread`), making this a data race.

**Fix:** Replaced both `strtok` calls with `strtok_r`, passing an explicit `save` pointer. Added `-D_POSIX_C_SOURCE=200809L` to `CFLAGS` in the Makefile (the project already depended on POSIX throughout; this makes it explicit and unlocks the full POSIX-2008 API).

---

## Integer Overflow — `atoi` → `strtol`

**Files:** `src/parser/cppcheck_parser.c`, `src/parser/flawfinder_parser.c`, `src/core/scoring.c`

`atoi` invokes undefined behaviour when the string value exceeds `INT_MAX`. All three sites parsed numeric fields from tool output files — data that could be crafted by a malicious tool or a corrupted output file.

**Fix:** Replaced every `atoi` call with `(int)strtol(..., NULL, 10)`. `strtol` is defined for all inputs; out-of-range values clamp to `LONG_MIN`/`LONG_MAX` with `ERANGE` rather than invoking UB.

---

## NULL Dereference — `str_starts_with`

**File:** `src/common/utils.c`

`str_starts_with(s, prefix)` passed `prefix` directly to `strlen` without a NULL check. `s` was guarded but `prefix` was not; a NULL `prefix` would segfault.

**Fix:** Added `if (!s || !prefix) return 0;` at the top of the function.

---

## Magic Buffer Bound in GUI Input

**File:** `src/gui/ui.c`

The keyboard input handler used the literal `510` as the upper bound for `m->target`, which is declared as `char target[512]` in `model.h`. Had the buffer size ever changed, the hardcoded `510` would silently become wrong (either an off-by-one overflow or a needlessly short limit).

**Fix:** Replaced `510` with `(int)sizeof(m->target) - 2`, making the bound self-documenting and automatically correct.

---

## Build Hardening Flags

The Makefile now includes targets for runtime sanitizers and static analysis. See [Running the Sanitizers](#running-the-sanitizers) below.

---

## Running the Sanitizers

Three opt-in build targets are available. Each does a clean rebuild with the relevant flags so no stale object files mix instrumented and uninstrumented code.

### AddressSanitizer + UndefinedBehaviorSanitizer

```
make asan
```

Enables `-fsanitize=address,undefined`. ASan inserts redzones around heap, stack, and global allocations and poisons freed memory; it catches buffer overflows, use-after-free, and out-of-bounds reads/writes at runtime. UBSan traps integer overflow, misaligned access, and other C undefined behaviour.

### ThreadSanitizer

```
make tsan
```

Enables `-fsanitize=thread`. Detects data races between the GUI's `analysis_thread` and the main UI thread. ASan and TSan are mutually exclusive; run them separately.

### GCC Static Analyzer

```
make analyze
```

Adds `-fanalyzer` to a normal (non-instrumented) build. GCC's inter-procedural static analyser checks for null-pointer dereferences, use-after-free, double-free, and other path-sensitive bugs at compile time without needing test input.

### Clang `scan-build`

```
make scan
```

Runs the Clang static analyzer via `scan-build`. Produces an HTML report in `build/scan-report/`. Requires `clang` and `scan-build` to be on `PATH`.
