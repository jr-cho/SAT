#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef enum {
    TOOL_CPPCHECK   = 0,
    TOOL_FLAWFINDER = 1,
    TOOL_GCC        = 2,
    TOOL_COVERITY   = 3,
    TOOL_COUNT      = 4
} ToolID;

typedef enum {
    SEV_UNKNOWN  = 0,
    SEV_INFO     = 1,
    SEV_LOW      = 2,
    SEV_MEDIUM   = 3,
    SEV_HIGH     = 4,
    SEV_CRITICAL = 5
} Severity;

typedef struct {
    ToolID   tool;
    char    *file;
    i32      line;
    i32      column;
    Severity severity;
    char    *category;
    char    *message;
} Finding;

#endif
