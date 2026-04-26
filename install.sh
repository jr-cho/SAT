#!/usr/bin/env bash
# SAT - Static Analysis Tool - Installer (Linux / macOS)
# Usage: bash install.sh
set -euo pipefail

# ── Terminal helpers ───────────────────────────────────────────────────────────
if [[ -t 1 ]]; then
    RED='\033[0;31m' GREEN='\033[0;32m' YELLOW='\033[1;33m'
    CYAN='\033[0;36m' BOLD='\033[1m' NC='\033[0m'
else
    RED='' GREEN='' YELLOW='' CYAN='' BOLD='' NC=''
fi

info()  { printf "  ${GREEN}[ok]${NC}  %s\n"    "$*"; }
warn()  { printf "  ${YELLOW}[!]${NC}   %s\n"   "$*"; }
error() { printf "  ${RED}[err]${NC}  %s\n"     "$*" >&2; }
step()  { printf "\n${BOLD}${CYAN}%s${NC}\n"    "$*"; }
die()   { error "$*"; exit 1; }
ask()   { printf "\n  %s " "$*"; }   # prompt helper

# ── Sanity check ───────────────────────────────────────────────────────────────
[[ -f "CMakeLists.txt" ]] || die "Run this script from the SAT project root directory."

# ── OS detection ───────────────────────────────────────────────────────────────
case "$(uname -s)" in
    Darwin) OS="macOS" ;;
    Linux)  OS="Linux" ;;
    *)      OS="$(uname -s)" ;;
esac

step "SAT — Static Analysis Tool — Installer"
printf "  Platform : %s %s\n" "$OS" "$(uname -m)"

# ── Check required tools ───────────────────────────────────────────────────────
step "Checking required tools"

has() { command -v "$1" &>/dev/null; }

if ! has cmake; then
    error "cmake not found. Install it:"
    [[ "$OS" == "macOS" ]] && printf "    brew install cmake\n"
    [[ "$OS" == "Linux"  ]] && printf "    sudo apt install cmake    # Debian / Ubuntu\n"
    [[ "$OS" == "Linux"  ]] && printf "    sudo pacman -S cmake      # Arch\n"
    [[ "$OS" == "Linux"  ]] && printf "    sudo dnf install cmake    # Fedora\n"
    exit 1
fi
info "cmake     $(cmake --version | head -1)"

COMPILER=""
if has gcc;   then COMPILER="gcc";   info "compiler  gcc $(gcc -dumpfullversion 2>/dev/null || gcc --version | awk '{print $3; exit}')"; fi
if has clang && [[ -z "$COMPILER" ]]; then
    COMPILER="clang"; info "compiler  $(clang --version | head -1)"
fi
[[ -z "$COMPILER" ]] && die "No C compiler found. Install gcc or clang."

GIT_OK=false
if has git; then
    GIT_OK=true
    info "git       $(git --version | awk '{print $3}')"
else
    warn "git not found — GUI target will be unavailable"
fi

# ── Raylib submodule status ────────────────────────────────────────────────────
RAYLIB_READY=false
if [[ -f "external/raylib/CMakeLists.txt" ]]; then
    RAYLIB_READY=true
    info "raylib    submodule present"
elif $GIT_OK; then
    warn "raylib    submodule not yet initialized (git can fetch it)"
else
    warn "raylib    submodule missing and git unavailable — GUI will be skipped"
fi

# ── Select what to build ───────────────────────────────────────────────────────
step "What would you like to build?"
printf "  1)  CLI only   (analyzer)\n"
if $RAYLIB_READY || $GIT_OK; then
    printf "  2)  CLI + GUI  (analyzer + sat-gui)\n"
fi
printf "  q)  Quit\n"
ask "Choice [1]:"; read -r t_choice
t_choice="${t_choice:-1}"

BUILD_GUI=false
case "$t_choice" in
    1) ;;
    2)
        if ! $RAYLIB_READY; then
            $GIT_OK || die "Git not available — cannot fetch the Raylib submodule."
            step "Fetching Raylib submodule..."
            git submodule add https://github.com/raysan5/raylib.git external/raylib 2>/dev/null || true
            git submodule update --init --recursive
            RAYLIB_READY=true
        fi
        BUILD_GUI=true
        ;;
    q|Q) printf "\nBye.\n"; exit 0 ;;
    *)   die "Invalid choice: $t_choice" ;;
esac

# ── Select build type ──────────────────────────────────────────────────────────
step "Build type"
printf "  1)  Release  — optimized, no debug info    (default)\n"
printf "  2)  Debug    — debug symbols, no optimization\n"
printf "  3)  ASan     — AddressSanitizer + UBSan    (mutually exclusive with TSan)\n"
printf "  4)  TSan     — ThreadSanitizer             (mutually exclusive with ASan)\n"
ask "Choice [1]:"; read -r b_choice
b_choice="${b_choice:-1}"

CMAKE_TYPE="Release"
CMAKE_ARGS=()
BUILD_DIR_SUFFIX="release"

case "$b_choice" in
    1) CMAKE_TYPE="Release"; BUILD_DIR_SUFFIX="release" ;;
    2) CMAKE_TYPE="Debug";   BUILD_DIR_SUFFIX="debug"   ;;
    3) CMAKE_TYPE="Debug";   BUILD_DIR_SUFFIX="asan";   CMAKE_ARGS+=(-DENABLE_ASAN=ON) ;;
    4) CMAKE_TYPE="Debug";   BUILD_DIR_SUFFIX="tsan";   CMAKE_ARGS+=(-DENABLE_TSAN=ON) ;;
    *) die "Invalid choice: $b_choice" ;;
esac

BUILD_DIR="build-${BUILD_DIR_SUFFIX}"
CMAKE_ARGS+=(-B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CMAKE_TYPE")

# ── Configure ─────────────────────────────────────────────────────────────────
step "Configuring (cmake)"
cmake "${CMAKE_ARGS[@]}" || die "CMake configure failed."

# ── Build ─────────────────────────────────────────────────────────────────────
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
step "Building with $JOBS parallel jobs"
cmake --build "$BUILD_DIR" --parallel "$JOBS" || die "Build failed."

# ── Summary ───────────────────────────────────────────────────────────────────
step "Build complete"
FOUND_BINS=()
for bin in analyzer sat-gui; do
    p="$BUILD_DIR/$bin"
    if [[ -f "$p" ]]; then
        info "$(realpath "$p")"
        FOUND_BINS+=("$p")
    fi
done

if [[ ${#FOUND_BINS[@]} -eq 0 ]]; then
    warn "No binaries found in $BUILD_DIR — check the build output above."
    exit 1
fi

# ── Optional system install ────────────────────────────────────────────────────
printf "\n"
ask "Install to /usr/local/bin? [y/N]:"; read -r inst_choice
if [[ "${inst_choice,,}" == "y" ]]; then
    for p in "${FOUND_BINS[@]}"; do
        sudo install -m 755 "$p" /usr/local/bin/ && info "Installed /usr/local/bin/$(basename "$p")"
    done
else
    printf "\n  To run manually:\n"
    for p in "${FOUND_BINS[@]}"; do
        printf "    %s\n" "$(realpath "$p")"
    done
fi

printf "\n"
