#include "../../include/common/logging.h"
#include <stdarg.h>

static LogLevel current_level = LOG_INFO;

static const char *level_str(LogLevel level)
{
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "?";
    }
}

void log_set_level(LogLevel level)
{
    current_level = level;
}

void log_msg(LogLevel level, const char *file, int line, const char *fmt, ...)
{
    if (level < current_level)
        return;

    FILE *out = (level >= LOG_WARN) ? stderr : stdout;

    fprintf(out, "[%s] %s:%d: ", level_str(level), file, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);

    fprintf(out, "\n");
}
