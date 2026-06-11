#ifndef LOGGER_H
#define LOGGER_H

typedef enum {
    LOGGER_LEVEL_ERROR = 0,
    LOGGER_LEVEL_INFO = 1,
    LOGGER_LEVEL_DEBUG = 2
} logger_level_t;

void logger_init(int verbosity);

void logger_log(logger_level_t level, const char *tag, const char *fmt, ...);

#define LOG_ERROR(tag, ...) logger_log(LOGGER_LEVEL_ERROR, tag, __VA_ARGS__)
#define LOG_INFO(tag, ...) logger_log(LOGGER_LEVEL_INFO, tag, __VA_ARGS__)
#define LOG_DEBUG(tag, ...) logger_log(LOGGER_LEVEL_DEBUG, tag, __VA_ARGS__)

#endif
