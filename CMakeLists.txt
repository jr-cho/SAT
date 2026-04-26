cmake_minimum_required(VERSION 3.16)
project(SAT C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── Compiler flags ─────────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /wd4996)          # 4996: suppress MSVC 'deprecated' noise
else()
    add_compile_options(-Wall -Wextra)
endif()
if(UNIX)
    add_compile_definitions(_POSIX_C_SOURCE=200809L)
endif()

# ── Sanitizer / analysis options ───────────────────────────────────────────────
# Usage:  cmake -DENABLE_ASAN=ON ..   (mutually exclusive with TSAN)
#         cmake -DENABLE_TSAN=ON ..
#         cmake -DENABLE_ANALYZE=ON ..
option(ENABLE_ASAN    "Build with AddressSanitizer + UBSan"  OFF)
option(ENABLE_TSAN    "Build with ThreadSanitizer"            OFF)
option(ENABLE_ANALYZE "Build with GCC -fanalyzer"             OFF)

if(ENABLE_ASAN AND NOT MSVC)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()
if(ENABLE_TSAN AND NOT MSVC)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=thread)
endif()
if(ENABLE_ANALYZE AND NOT MSVC)
    add_compile_options(-fanalyzer)
endif()

# ── Shared sources (CLI + GUI) ─────────────────────────────────────────────────
set(SRCS_SHARED
    src/common/logging.c
    src/common/utils.c
    src/core/correlation.c
    src/core/database.c
    src/core/findings.c
    src/core/scoring.c
    src/parser/parser.c
    src/parser/cppcheck_parser.c
    src/parser/flawfinder_parser.c
    src/parser/gcc_parser.c
    src/parser/coverity_parser.c
    src/runner/runner.c
    src/runner/cppcheck.c
    src/runner/flawfinder.c
    src/runner/gcc.c
    src/runner/coverity.c
    src/report/report.c
    src/report/json.c
    src/tool_finder.c
)

# ── CLI target ─────────────────────────────────────────────────────────────────
add_executable(analyzer ${SRCS_SHARED} src/main.c)
target_include_directories(analyzer PRIVATE include src)

# ── GUI target ─────────────────────────────────────────────────────────────────
# Requires the Raylib submodule.  One-time setup:
#   git submodule add https://github.com/raysan5/raylib.git external/raylib
#   git submodule update --init --recursive
#
# Threading uses C11 <threads.h>:
#   GCC 4.9+  |  Clang 3.6+  |  MSVC VS 2022 17.8+  |  MinGW (bundled)
find_package(Threads REQUIRED)

if(EXISTS "${CMAKE_SOURCE_DIR}/external/raylib/CMakeLists.txt")
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/raylib EXCLUDE_FROM_ALL)

    add_executable(sat-gui
        ${SRCS_SHARED}
        src/gui/model.c
        src/gui/controller.c
        src/gui/ui.c
        src/gui/main.c
    )
    target_include_directories(sat-gui PRIVATE include src)
    target_link_libraries(sat-gui PRIVATE raylib Threads::Threads)

    if(WIN32)
        # Raylib needs winmm for timeBeginPeriod on Windows
        target_link_libraries(sat-gui PRIVATE winmm)
    endif()

    message(STATUS "sat-gui enabled — Raylib submodule found")
else()
    message(STATUS "sat-gui disabled — Raylib submodule missing.")
    message(STATUS "  Run: git submodule add https://github.com/raysan5/raylib.git external/raylib")
    message(STATUS "       git submodule update --init --recursive")
endif()
