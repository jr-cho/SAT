CC         = gcc
CFLAGS     = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude -Isrc $(EXTRA_FLAGS)
LDFLAGS    = $(EXTRA_FLAGS)

RAYLIB_INC  = -I/opt/homebrew/include
RAYLIB_LIBS = -L/opt/homebrew/lib -lraylib \
              -framework OpenGL -framework Cocoa \
              -framework IOKit -framework CoreVideo

BUILD_DIR = build
OBJ_DIR   = $(BUILD_DIR)/obj
TARGET    = $(BUILD_DIR)/analyzer
GUI_TARGET = $(BUILD_DIR)/sat-gui

SRCS_COMMON = \
    src/common/logging.c \
    src/common/utils.c

SRCS_CORE = \
    src/core/correlation.c \
    src/core/database.c \
    src/core/findings.c \
    src/core/scoring.c

SRCS_GUI = \
    src/gui/model.c \
    src/gui/controller.c \
    src/gui/ui.c \
    src/gui/main.c

SRCS_PARSER = \
    src/parser/parser.c \
    src/parser/cppcheck_parser.c \
    src/parser/flawfinder_parser.c \
    src/parser/gcc_parser.c \
    src/parser/coverity_parser.c

SRCS_RUNNER = \
    src/runner/runner.c \
    src/runner/cppcheck.c \
    src/runner/flawfinder.c \
    src/runner/gcc.c \
    src/runner/coverity.c

SRCS_REPORT = \
    src/report/report.c \
    src/report/json.c

SRCS_MAIN = \
    src/tool_finder.c \
    src/main.c

SRCS      = $(SRCS_COMMON) $(SRCS_CORE) $(SRCS_PARSER) $(SRCS_RUNNER) $(SRCS_REPORT) $(SRCS_MAIN)
OBJS      = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# GUI: shared objects are everything except main.o
SHARED_OBJS = $(filter-out $(OBJ_DIR)/main.o, $(OBJS))
GUI_OBJS    = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SRCS_GUI))

.PHONY: all gui run clean asan tsan analyze scan

all: $(TARGET)

gui: $(GUI_TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

# GUI-specific rule (shorter stem wins over the generic pattern)
$(OBJ_DIR)/gui/%.o: src/gui/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(RAYLIB_INC) -c $< -o $@

$(GUI_TARGET): $(GUI_OBJS) $(SHARED_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(RAYLIB_LIBS)

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(GUI_TARGET)

# AddressSanitizer + UndefinedBehaviorSanitizer (redzones, poison freed memory, UB traps)
asan:
	$(MAKE) clean all EXTRA_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g"

# ThreadSanitizer — detects races between analysis_thread and the UI thread
# NOTE: mutually exclusive with asan; run separately
tsan:
	$(MAKE) clean all EXTRA_FLAGS="-fsanitize=thread -fno-omit-frame-pointer -g"

# GCC inter-procedural static analyzer (no instrumentation, catches bugs at compile time)
analyze:
	$(MAKE) clean all EXTRA_FLAGS="-fanalyzer"

# Clang static analyzer — produces HTML report in build/scan-report/
# Requires clang + scan-build on PATH
scan:
	scan-build -o build/scan-report $(MAKE) clean all CC=clang
