# SAT — Static Analysis Tool

SAT runs multiple C static analysis tools against a source file, normalises their output into a unified finding model, correlates overlapping results across tools, and renders a report. It ships as both a command-line tool (`analyzer`) and a Raylib-based GUI (`sat-gui`).

```
$ ./analyzer tests/test.c

  Static Analysis Report
  Target : tests/test.c
  Date   : 2026-04-26 14:32:01
======================================================================

SUMMARY
----------------------------------------------------------------------
Tool                  Total   Info    Low  Medium   High   Critical
----------------------------------------------------------------------
cppcheck                  3      0      0       1      2          0
flawfinder                2      0      1       0      1          0
gcc (-fanalyzer)          0      0      0       0      0          0
coverity                  0      0      0       0      0          0
----------------------------------------------------------------------
TOTAL                     5      0      1       1      3          0
```

---

## Documentation

| Document | Description |
|----------|-------------|
| [docs/install.md](docs/install.md) | Prerequisites and build instructions for Linux, macOS, and Windows |
| [docs/architecture.md](docs/architecture.md) | File-by-file reference — what every source file does |
| [docs/design.md](docs/design.md) | Design decisions — data model, pipeline, algorithms, trade-offs |
| [docs/TOOLS.md](docs/TOOLS.md) | The four wrapped tools — invocation, output format, severity mapping, installation |
| [docs/security.md](docs/security.md) | Security hardening — fixes applied and how to run sanitizers |

---

## Quick Start

**Linux / macOS**
```bash
git clone <repository-url>
cd SAT
bash install.sh
```

**Windows (PowerShell)**
```powershell
git clone <repository-url>
cd SAT
.\install.ps1
```

Both scripts detect your compiler and toolchain, offer CLI-only or CLI + GUI build options, and print the path to the resulting binary.

---

## What It Does

1. **Runs tools** — forks `cppcheck`, `flawfinder`, `gcc -fanalyzer`, and `cov-analyze` as child processes, capturing their output to temp files.
2. **Parses output** — each tool has a dedicated parser that converts XML, CSV, or text output into `Finding` structs (file, line, column, severity, category, message).
3. **Correlates findings** — findings from different tools within three lines of each other in the same file are grouped into a `CorrelatedFinding`. Multi-tool hits appear with a confidence score.
4. **Reports** — writes a formatted text report and a JSON file. The CLI prints the text report to stdout. The GUI displays findings in a filterable list with a detail panel.

All four tools are optional. SAT runs and reports whatever is found by whichever tools are installed.

---

## Supported Tools

| Tool | What it detects | Required? |
|------|----------------|-----------|
| [Cppcheck](https://cppcheck.sourceforge.io) | Memory errors, undefined behaviour, coding issues | No |
| [Flawfinder](https://dwheeler.com/flawfinder/) | Dangerous function calls with CWE references | No |
| [GCC `-fanalyzer`](https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html) | Use-after-free, null deref, taint analysis | No (requires GCC 12+) |
| [Coverity](https://scan.coverity.com) | Deep inter-procedural analysis | No (commercial licence) |

---

## Building

The recommended path is the install script above. For manual CMake builds:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Add `-DENABLE_ASAN=ON` for AddressSanitizer + UBSan, `-DENABLE_TSAN=ON` for ThreadSanitizer. See [docs/install.md](docs/install.md) for the full options.

The GUI requires the Raylib submodule:

```bash
git submodule add https://github.com/raysan5/raylib.git external/raylib
git submodule update --init --recursive
cmake -B build && cmake --build build
```

---

## CLI Usage

```
analyzer <source_file.c>
```

Outputs to stdout and writes `analysis_report.txt` and `analysis_report.json` in the current directory.

## GUI Usage

```
sat-gui
```

Enter a source file path in the top bar and click **Run Analysis** (or press Enter). Use the sidebar to filter by tool or minimum severity. Click a finding row to see the full message in the detail panel.

---

## License

SAT itself is released under the terms in `LICENSE`.

Bundled / wrapped tools have separate licences:

| Tool | Licence |
|------|---------|
| [Raylib](https://github.com/raysan5/raylib) | zlib |
| [Cppcheck](https://cppcheck.sourceforge.io) | GPL v3 |
| [Flawfinder](https://dwheeler.com/flawfinder/) | GPL v2 |
| GCC | GPL v3 |
| Coverity | Commercial — Synopsys |
