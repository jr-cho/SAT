CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude -Isrc
LDFLAGS =

BUILD_DIR = build
OBJ_DIR   = $(BUILD_DIR)/obj
TARGET    = $(BUILD_DIR)/analyzer

SRCS_COMMON = \
    src/common/logging.c \
    src/common/utils.c

SRCS_CORE = \
    src/core/correlation.c \
    src/core/database.c \
    src/core/findings.c \
    src/core/scoring.c

// NEED TO ADD GUI -JG

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

SRCS = $(SRCS_COMMON) $(SRCS_CORE) $(SRCS_PARSER) $(SRCS_RUNNER) $(SRCS_REPORT) $(SRCS_MAIN)
OBJS = $(patsubst src/%.c, $(OBJ_DIR)/%.o, $(SRCS))

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
