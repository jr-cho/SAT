# External Tools

SAT wraps four static analysis tools. All four are optional — SAT will run and report findings from whichever tools it can locate. Tools are discovered at runtime; none are bundled with SAT.

---

## Cppcheck

**Type:** Open source | **License:** GPL v3

Cppcheck performs static analysis focused on bug detection: null pointer dereferences, out-of-bounds access, memory leaks, uninitialised variables, and coding standard violations. It does not require a successful compilation and can analyse code with missing headers.

### How SAT invokes it

```
cppcheck --xml --xml-version=2 --enable=all --suppress=missingIncludeSystem -- <file>
```

SAT reads the XML v2 output written to a temp file. Each `<error>` element becomes one `Finding`; the first `<location>` child supplies the file, line, and column.

### Severity mapping

| Cppcheck | SAT |
|----------|-----|
| `error` | HIGH |
| `warning` | MEDIUM |
| `style` | LOW |
| `performance` | LOW |
| `information` | INFO |

### Installation

| Platform | Command |
|----------|---------|
| Ubuntu / Debian | `sudo apt install cppcheck` |
| Arch | `sudo pacman -S cppcheck` |
| Fedora | `sudo dnf install cppcheck` |
| macOS | `brew install cppcheck` |
| Windows | Installer at [cppcheck.sourceforge.io](https://cppcheck.sourceforge.io) — add `bin/` to `PATH` |

### Reference

- Manual: <https://cppcheck.sourceforge.io/manual.pdf>
- Checker list: <https://sourceforge.net/p/cppcheck/wiki/ListOfChecks>

---

## Flawfinder

**Type:** Open source | **License:** GPL v2

Flawfinder scans C/C++ source for calls to functions known to have security risks (e.g. `strcpy`, `gets`, `sprintf`). Each hit is annotated with a risk level (0–5) and, where applicable, a CWE identifier. It operates entirely via text matching — no compilation required.

### How SAT invokes it

```
flawfinder --dataonly --csv -- <file>
```

`--dataonly` suppresses the header/footer prose. SAT reads the CSV output and maps columns by index (file, line, column, level, category, name, warning, CWEs).

### Severity mapping

| Flawfinder level | SAT |
|-----------------|-----|
| 5 | CRITICAL |
| 4 | HIGH |
| 3 | MEDIUM |
| 2 | LOW |
| 1 | INFO |

### Installation

Flawfinder is a Python package, installable on all platforms:

```bash
pip install flawfinder
```

On Debian / Ubuntu it is also available via apt:

```bash
sudo apt install flawfinder
```

### Reference

- Homepage: <https://dwheeler.com/flawfinder/>
- CWE list: <https://cwe.mitre.org>

---

## GCC `-fanalyzer`

**Type:** Open source | **License:** GPL v3

GCC's `-fanalyzer` flag enables an inter-procedural static analysis pass that models program state along execution paths. It detects use-after-free, double-free, null dereference, memory leaks, format string mismatches, and tainted data reaching sensitive sinks. It requires the source to compile cleanly (or near-cleanly).

### How SAT invokes it

```
<gcc> -fanalyzer -o /dev/null -- <file>
```

SAT captures `stderr` (where GCC writes diagnostics) and parses lines matching the format:

```
<file>:<line>:<col>: warning: <message> [CWE-NNN] [-Wanalyzer-flag]
```

Lines that do not match this pattern (source context, event traces) are ignored.

### Severity mapping

| GCC diagnostic | SAT |
|---------------|-----|
| `error` | HIGH |
| `warning` | MEDIUM |
| `note` | INFO |

### Tool discovery

SAT searches for GCC in this order:

1. `GCC_ANALYZER` environment variable (if set and executable)
2. `gcc-15`, `gcc-14`, `gcc-13` in `PATH`
3. `/opt/homebrew/bin/gcc-15` … `/opt/homebrew/bin/gcc-13` (macOS Homebrew)
4. `/usr/local/bin/gcc-15` … `/usr/local/bin/gcc-13`
5. Plain `gcc` in `PATH`

Note: on macOS, `gcc` is typically a symlink to Apple Clang, which does not support `-fanalyzer`. Install a versioned GCC via Homebrew (`brew install gcc@14`) and SAT will find it automatically.

### Requirements

GCC 12 or later. Earlier versions either lack `-fanalyzer` or have significantly fewer checks.

### Reference

- Options: <https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html>
- Analyzer internals: <https://gcc.gnu.org/onlinedocs/gccint/Analyzer.html>

---

## Coverity

**Type:** Commercial | **Vendor:** Synopsys

Coverity performs deep inter-procedural analysis across entire codebases. It models data flow, control flow, and concurrency to find defects that simpler tools miss. A Coverity licence is required; free access for open-source projects is available via [scan.coverity.com](https://scan.coverity.com).

### How SAT invokes it

Coverity requires a two-step process. SAT only automates step 2; step 1 must be run manually beforehand:

```bash
# Step 1 (manual — run once per build)
cov-build --dir cov-int <your-build-command>

# Step 2 (run by SAT automatically)
cov-analyze --dir cov-int -- <file>
```

SAT emits a warning if Coverity is invoked without a prior build capture. Results may be incomplete or empty in that case.

Output is parsed in pairs of lines:

1. A CID line: `<file>:<line>:  CID <id> (#N of M): <Category> (<CHECKER>):`
2. A severity event line: `<file>:<line>:  <high|medium|low>: <event>: <message>`

### Severity mapping

| Coverity | SAT |
|----------|-----|
| `high` | HIGH |
| `medium` | MEDIUM |
| `low` | LOW |

### Installation

Download `cov-analysis` from your [Synopsys portal](https://scan.coverity.com). Add the `bin/` subdirectory to your `PATH` so that `cov-build` and `cov-analyze` are reachable.

### Reference

- Synopsys documentation portal: <https://documentation.blackduck.com>
- Free open-source scanning: <https://scan.coverity.com>
