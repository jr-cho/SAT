# Installation Guide

SAT builds and runs on Linux, macOS, and Windows. The fastest path on any platform is the provided install script, which detects your toolchain, fetches the Raylib submodule if needed, and invokes CMake for you.

---

## Quick Start

| Platform | Command |
|----------|---------|
| Linux / macOS | `bash install.sh` |
| Windows (PowerShell) | `.\install.ps1` |

Both scripts present a menu to choose CLI-only or CLI + GUI, and Release / Debug / sanitizer builds. See the script output for the binary location.

> **Note — first-time Windows PowerShell users:** if you see an execution policy error, run once as Administrator then retry:
> ```powershell
> Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
> ```

---

## Prerequisites

### 1. Build tools

These must be on your `PATH` before running the install script.

#### Linux

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake git

# Arch
sudo pacman -S base-devel cmake git

# Fedora
sudo dnf install gcc cmake git
```

#### macOS

Install [Homebrew](https://brew.sh) if you do not already have it, then:

```bash
brew install cmake git gcc
```

The install script uses the system `gcc` or whichever `gcc-14` / `gcc-15` is first on `PATH`. If you want `-fanalyzer` support, install a numbered GCC version (`brew install gcc@14`).

#### Windows

**Option A — MinGW via MSYS2 (recommended)**

1. Download and install [MSYS2](https://www.msys2.org)
2. Open the MSYS2 MinGW 64-bit shell and run:
   ```
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-git
   ```
3. Add `C:\msys64\mingw64\bin` to your Windows `PATH`

**Option B — Visual Studio**

Install [Visual Studio 2022](https://visualstudio.microsoft.com) with the **Desktop development with C++** workload. The GUI target requires VS 2022 v17.8 or later (needed for C11 `<threads.h>`).

CMake and git can be installed separately:
```
winget install Kitware.CMake Git.Git
```

---

### 2. Static analysis tools (optional but recommended)

SAT wraps these tools. The CLI and GUI will still run without them — they simply report no findings for any tool that is not found.

#### Cppcheck

| Platform | Command |
|----------|---------|
| Ubuntu / Debian | `sudo apt install cppcheck` |
| Arch | `sudo pacman -S cppcheck` |
| Fedora | `sudo dnf install cppcheck` |
| macOS | `brew install cppcheck` |
| Windows | Download the installer from [cppcheck.sourceforge.io](https://cppcheck.sourceforge.io) |

#### Flawfinder

Flawfinder is a Python tool. Install it via pip on all platforms:

```bash
pip install flawfinder
# or: pip3 install flawfinder
```

Alternatively, on Debian/Ubuntu: `sudo apt install flawfinder`

#### GCC `-fanalyzer`

`-fanalyzer` requires GCC 12 or later. The version already installed by the build steps above is usually sufficient. SAT searches `gcc-15`, `gcc-14`, `gcc-13`, and plain `gcc` in order.

To check whether your GCC supports it:
```bash
gcc --version                       # must be 12+
echo "" | gcc -fanalyzer -x c -     # should produce no "unrecognized option" error
```

#### Coverity

Coverity is a commercial product. Download `cov-analysis` from your [Synopsys portal](https://scan.coverity.com) and add the `bin/` directory to your `PATH`. SAT calls `cov-analyze` and expects a prior `cov-build` capture directory named `cov-int/` in the working directory.

---

## Manual Build (without the install script)

If you prefer to drive CMake directly:

```bash
# Clone
git clone <repository-url>
cd SAT

# CLI only
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# CLI + GUI (requires Raylib submodule)
git submodule add https://github.com/raysan5/raylib.git external/raylib
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

On Windows with MinGW, pass `-G "MinGW Makefiles"` to cmake. With Visual Studio, omit `-G` (CMake auto-detects).

---

## Sanitizer / Analysis Builds

These are for development and debugging. Invoke via CMake options or the install script's build-type menu.

| CMake flag | What it enables |
|-----------|----------------|
| `-DENABLE_ASAN=ON` | AddressSanitizer + UBSan — redzones, use-after-free detection, UB traps |
| `-DENABLE_TSAN=ON` | ThreadSanitizer — data race detection (mutually exclusive with ASan) |
| `-DENABLE_ANALYZE=ON` | GCC `-fanalyzer` — compile-time inter-procedural analysis |

With the Makefile (Linux / macOS CLI builds only):

```bash
make asan      # ASan + UBSan
make tsan      # ThreadSanitizer
make analyze   # GCC -fanalyzer
make scan      # Clang scan-build (requires clang + scan-build on PATH)
```

---

## Verifying the Install

```bash
# CLI
./build/analyzer tests/test.c

# Expected output: a formatted report listing the intentional vulnerabilities
# in tests/test.c (strcpy overflow, memory leak, uninitialized variable)
```
