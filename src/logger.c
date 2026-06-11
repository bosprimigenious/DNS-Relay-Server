#include "logger.h"

#include <stdarg.h>
#include <stdio.h>

static int g_logger_verbosity = 0;

void logger_init(int verbosity) {
    if (verbosity < 0) {
        verbosity = 0;
    }
    if (verbosity > 2) {
        verbosity = 2;
    }
    g_logger_verbosity = verbosity;
}

void logger_log(logger_level_t level, const char *tag, const char *fmt, ...) {
    const char *level_name;
    va_list args;

    if ((int)level > g_logger_verbosity && level != LOGGER_LEVEL_ERROR) {
        return;
    }

    switch (level) {
    case LOGGER_LEVEL_ERROR:
        level_name = "ERROR";
        break;
    case LOGGER_LEVEL_DEBUG:
        level_name = "DEBUG";
        break;
    case LOGGER_LEVEL_INFO:
    default:
        level_name = "INFO";
        break;
    }

    fprintf(stderr, "[%s] [%s] ", level_name, tag);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}
