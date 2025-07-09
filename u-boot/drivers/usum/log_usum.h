#ifndef USUM_LOG_H
#define USUM_LOG_H

#define USUM_LOG_INFO  0
#define USUM_LOG_WARN  1
#define USUM_LOG_ERROR 2
#define USUM_LOG_DEBUG 3

#ifndef CONFIG_USUM_LOG_LEVEL
#define CONFIG_USUM_LOG_LEVEL USUM_LOG_INFO
#endif

void usum_log(int level, const char *fmt, ...);

#define USUM_LOG(level, ...) \
    do { \
        if ((level) <= CONFIG_USUM_LOG_LEVEL) \
            usum_log((level), __VA_ARGS__); \
    } while (0)

#endif // USUM_LOG_H
