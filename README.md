# Software Framework for Comparative Analysis of Source Code Static Analysis Tools

A GUI-based framework that runs multiple C/C++ static analysis tools simultaneously and generates unified vulnerability reports.

## Overview

This framework integrates four static analysis tools to compare their findings across the same codebase:

- **Cppcheck** — detects memory leaks, undefined behavior, and coding standard violations
- **Flawfinder** — identifies potential security vulnerabilities with CWE references
- **Coverity** — performs deep static analysis for security defects and reliability issues
- **GCC (`-fanalyzer`)** — captures compiler warnings and semantic issues

## Features

- GUI file selector for one or more `.c`, `.cpp`, `.h`, or `.hpp` files
- Parallel or sequential tool execution with progress indicators
- Unified report with severity levels (low, medium, high, critical), file names, and line numbers
- Filtering by severity, tool, or keyword
- Export to PDF, HTML, JSON, or XML
- Configurable per-tool settings (rule sets, risk thresholds, warning levels)
- Offline — no internet connection required

## Requirements

### System

| Requirement | Minimum |
|-------------|---------|
| CPU | x86-64, 4 cores recommended |
| RAM | 8 GB |
| Storage | SSD recommended |

### Operating System

- **Linux** — Ubuntu 22.04+ (recommended)
- **macOS** — 12+
- **Windows** — Windows 10/11 via [WSL](https://learn.microsoft.com/en-us/windows/wsl/install)

### Dependencies

- GCC 11+
- Cppcheck 2.x
- Flawfinder 2.x
- Coverity 2023.x+
- Clang
- Raylib

## Installation

1. Clone the repository:
   ```bash
   git clone <repository-url>
   cd <repository-directory>
   ```

2. Install dependencies (Ubuntu):
   ```bash
   sudo apt update
   sudo apt install gcc cppcheck flawfinder clang
   ```
   Coverity requires a separate download from the [Synopsys website](https://scan.coverity.com/).

3. Build the project:
   ```bash
   make
   ```

### Windows (WSL)

Install WSL before proceeding:
```powershell
wsl --install
```
Then follow the Linux instructions inside the WSL terminal.

## Usage

1. Launch the application
2. Click **Select File** to choose one or more `.c`/`.cpp` files, or select a directory
3. (Optional) Click **Settings** to configure tool-specific options
4. Click **Run Analysis**
5. Click **View Report** to review findings
6. (Optional) Click **Export Report** to save as PDF, HTML, JSON, or XML

## Supported File Types

| Extension | Description |
|-----------|-------------|
| `.c` | C source file |
| `.cpp` | C++ source file |
| `.h` | C/C++ header |
| `.hpp` | C++ header |

## Report Output

Reports include:

- Total issue count and severity distribution
- Per-file findings with line numbers
- CWE IDs and rule references where available
- Tool-specific results alongside a normalized cross-tool view

## Performance Targets

| Metric | Target |
|--------|--------|
| UI response time | ≤ 200 ms |
| Analysis start | ≤ 2 seconds |
| 100-file project analysis | ≤ 60 seconds |
| Report display | ≤ 3 seconds |
| Max RAM (≤1,000 files) | ≤ 2 GB |

## Security

- File inputs restricted to `.c`, `.cpp`, `.h`, `.hpp` with enforced size limits
- File paths and user inputs sanitized against command injection and path traversal
- External tools run in controlled subprocess environments
- Source code is not transmitted outside the local environment

## License

This project includes the following open-source tools. See the **About** section of the application for their respective license notices:

- [Cppcheck](https://cppcheck.sourceforge.io/) — GPL v3
- [Flawfinder](https://dwheeler.com/flawfinder/) — GPL v2
- [GCC](https://gcc.gnu.org/) — GPL v3
- [Raylib](https://www.raylib.com/) — zlib

Coverity is a commercial product by Synopsys.

## Contributing

The system is designed with a modular plugin architecture. New static analysis tools can be integrated by implementing the standardized adapter interface without modifying core logic. See the developer documentation for details.
