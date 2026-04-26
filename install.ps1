#Requires -Version 5.1
# SAT - Static Analysis Tool - Installer (Windows)
# Usage (from the SAT project root in PowerShell):
#   .\install.ps1
#
# If you see "execution policy" errors, run once as Administrator:
#   Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

# ── Output helpers ─────────────────────────────────────────────────────────────
function Write-Step { Write-Host "`n$args" -ForegroundColor Cyan }
function Write-OK   { Write-Host "  [ok]  $args" -ForegroundColor Green }
function Write-Warn { Write-Host "  [!]   $args" -ForegroundColor Yellow }
function Write-Fail { Write-Host "  [err] $args" -ForegroundColor Red }
function Abort {
    param([string]$Msg)
    Write-Fail $Msg
    exit 1
}
function Ask {
    param([string]$Prompt)
    Write-Host ""
    return (Read-Host "  $Prompt")
}

# ── Sanity check ───────────────────────────────────────────────────────────────
if (-not (Test-Path "CMakeLists.txt")) {
    Abort "Run this script from the SAT project root directory."
}

# ── Header ─────────────────────────────────────────────────────────────────────
Write-Step "SAT - Static Analysis Tool - Installer"
$osVer = [System.Environment]::OSVersion.Version
Write-Host "  Platform : Windows $osVer ($env:PROCESSOR_ARCHITECTURE)"

# ── Check required tools ───────────────────────────────────────────────────────
Write-Step "Checking required tools"

function Find-Tool { param([string]$Name); Get-Command $Name -ErrorAction SilentlyContinue }

if (-not (Find-Tool cmake)) {
    Write-Fail "cmake not found."
    Write-Host "  Install options:"
    Write-Host "    winget install Kitware.CMake"
    Write-Host "    https://cmake.org/download/"
    exit 1
}
Write-OK "cmake      $((cmake --version)[0])"

# Detect compiler — MSVC (cl.exe) takes priority, then MinGW gcc
$Compiler = ""
$Generator = ""

if (Find-Tool cl) {
    $Compiler  = "msvc"
    $Generator = ""    # Empty = let CMake auto-detect the installed Visual Studio version
    Write-OK "compiler   MSVC (cl.exe)"
    Write-Warn "GUI requires Visual Studio 2022 v17.8+ for C11 <threads.h>"
} elseif (Find-Tool gcc) {
    $Compiler  = "mingw"
    $Generator = "MinGW Makefiles"
    Write-OK "compiler   MinGW GCC  $((gcc --version)[0])"
} elseif (Find-Tool clang) {
    $Compiler  = "clang"
    $Generator = "MinGW Makefiles"
    Write-OK "compiler   Clang  $((clang --version)[0])"
} else {
    Write-Fail "No C compiler found."
    Write-Host ""
    Write-Host "  Option A — MinGW via MSYS2 (recommended for open-source builds):"
    Write-Host "    1. Download MSYS2 from https://www.msys2.org"
    Write-Host "    2. Run:  pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake"
    Write-Host "    3. Add C:\msys64\mingw64\bin to your PATH"
    Write-Host ""
    Write-Host "  Option B — Visual Studio:"
    Write-Host "    Install 'Desktop development with C++' workload from VS Installer"
    exit 1
}

$GitOK = $false
if (Find-Tool git) {
    $GitOK = $true
    Write-OK "git        $((git --version))"
} else {
    Write-Warn "git not found — GUI target will be unavailable"
    Write-Host "           Download: https://git-scm.com/download/win"
}

# ── Raylib submodule status ────────────────────────────────────────────────────
$RaylibReady = $false
if (Test-Path "external\raylib\CMakeLists.txt") {
    $RaylibReady = $true
    Write-OK "raylib     submodule present"
} elseif ($GitOK) {
    Write-Warn "raylib     submodule not yet initialized (git can fetch it)"
} else {
    Write-Warn "raylib     submodule missing and git unavailable — GUI will be skipped"
}

# ── Select what to build ───────────────────────────────────────────────────────
Write-Step "What would you like to build?"
Write-Host "  1)  CLI only   (analyzer.exe)"
if ($RaylibReady -or $GitOK) {
    Write-Host "  2)  CLI + GUI  (analyzer.exe + sat-gui.exe)"
}
Write-Host "  q)  Quit"
$tChoice = Ask "Choice [1]"
if ([string]::IsNullOrEmpty($tChoice)) { $tChoice = "1" }

$BuildGUI = $false
switch ($tChoice) {
    "1" { }
    "2" {
        if (-not $RaylibReady) {
            if (-not $GitOK) { Abort "Git not available — cannot fetch the Raylib submodule." }
            Write-Step "Fetching Raylib submodule..."
            # submodule add is a no-op if already registered; suppress the error
            git submodule add https://github.com/raysan5/raylib.git external/raylib 2>$null
            git submodule update --init --recursive
            if ($LASTEXITCODE -ne 0) { Abort "Failed to initialize Raylib submodule." }
            $RaylibReady = $true
        }
        $BuildGUI = $true
    }
    { $_ -in @("q", "Q") } { Write-Host "`nBye."; exit 0 }
    default { Abort "Invalid choice: $tChoice" }
}

# ── Select build type ──────────────────────────────────────────────────────────
Write-Step "Build type"
Write-Host "  1)  Release  — optimized, no debug info    (default)"
Write-Host "  2)  Debug    — debug symbols, no optimization"
if ($Compiler -ne "msvc") {
    Write-Host "  3)  ASan     — AddressSanitizer + UBSan    (mutually exclusive with TSan)"
    Write-Host "  4)  TSan     — ThreadSanitizer             (mutually exclusive with ASan)"
}
$bChoice = Ask "Choice [1]"
if ([string]::IsNullOrEmpty($bChoice)) { $bChoice = "1" }

$CMakeType      = "Release"
$BuildDirSuffix = "release"
$ExtraFlags     = @()

switch ($bChoice) {
    "1" { $CMakeType = "Release"; $BuildDirSuffix = "release" }
    "2" { $CMakeType = "Debug";   $BuildDirSuffix = "debug"   }
    "3" {
        if ($Compiler -eq "msvc") { Abort "ASan is not supported with MSVC in this build setup." }
        $CMakeType = "Debug"; $BuildDirSuffix = "asan"; $ExtraFlags = @("-DENABLE_ASAN=ON")
    }
    "4" {
        if ($Compiler -eq "msvc") { Abort "TSan is not supported with MSVC in this build setup." }
        $CMakeType = "Debug"; $BuildDirSuffix = "tsan"; $ExtraFlags = @("-DENABLE_TSAN=ON")
    }
    default { Abort "Invalid choice: $bChoice" }
}

$BuildDir = "build-$BuildDirSuffix"

# ── Configure ─────────────────────────────────────────────────────────────────
Write-Step "Configuring (cmake)"
# Remove any stale cache from a previous run or a different machine's path.
if (Test-Path $BuildDir) {
    Write-Warn "Removing stale build dir: $BuildDir"
    Remove-Item -Recurse -Force $BuildDir
}

$CmakeArgs = @("-B", $BuildDir, "-DCMAKE_BUILD_TYPE=$CMakeType") + $ExtraFlags
if ($Generator) {
    $CmakeArgs = @("-G", $Generator) + $CmakeArgs
}

& cmake @CmakeArgs
if ($LASTEXITCODE -ne 0) { Abort "CMake configure failed." }

# ── Build ─────────────────────────────────────────────────────────────────────
$Cores = [System.Environment]::ProcessorCount
Write-Step "Building with $Cores parallel jobs"
& cmake --build $BuildDir --config $CMakeType --parallel $Cores
if ($LASTEXITCODE -ne 0) { Abort "Build failed." }

# ── Summary ───────────────────────────────────────────────────────────────────
Write-Step "Build complete"

# MSVC places binaries in $BuildDir\$CMakeType\; MinGW/Clang place them in $BuildDir\
$SearchPaths = @($BuildDir, (Join-Path $BuildDir $CMakeType))
$FoundBins   = @()

foreach ($dir in $SearchPaths) {
    foreach ($name in @("analyzer.exe", "sat-gui.exe")) {
        $full = Join-Path $dir $name
        if (Test-Path $full) {
            Write-OK (Resolve-Path $full).Path
            $FoundBins += (Resolve-Path $full).Path
        }
    }
}

if ($FoundBins.Count -eq 0) {
    Write-Warn "No binaries found — check the build output above."
    exit 1
}

Write-Host ""
Write-Host "  To run the CLI:" -ForegroundColor Cyan
Write-Host "    $BuildDir\analyzer.exe <source_file.c>"
if ($BuildGUI) {
    Write-Host "  To run the GUI:" -ForegroundColor Cyan
    Write-Host "    $BuildDir\sat-gui.exe"
}
Write-Host ""
