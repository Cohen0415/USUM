/*
 * Copyright (C) 2025 Cohen0415
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "log_usum.h"
#include <stdio.h>
#include <stdarg.h>

static const char *usum_log_level_str(int level)
{
    switch (level)
    {
    case USUM_LOG_INFO:
        return "INFO";
    case USUM_LOG_WARN:
        return "WARN";
    case USUM_LOG_ERROR:
        return "ERROR";
    case USUM_LOG_DEBUG:
        return "DEBUG";
    default:
        return "UNKNOWN";
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
