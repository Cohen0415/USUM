#include "log_usum.h"
#include <stdio.h>
#include <stdarg.h>

static const char *usum_log_level_str(int level) 
{
    switch (level) 
    {
        case USUM_LOG_INFO:  return "INFO";
        case USUM_LOG_WARN:  return "WARN";
        case USUM_LOG_ERROR: return "ERROR";
        case USUM_LOG_DEBUG: return "DEBUG";
        default:             return "UNKNOWN";
    }
}

void usum_log(int level, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    const char *level_str = usum_log_level_str(level);
    printf("USUM [%s] : ", level_str);
    vprintf(fmt, args);

    va_end(args);
}
