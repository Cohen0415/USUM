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

#ifndef USUM_LOG_H
#define USUM_LOG_H

#define USUM_LOG_INFO 0
#define USUM_LOG_WARN 1
#define USUM_LOG_ERROR 2
#define USUM_LOG_DEBUG 3

#ifndef CONFIG_USUM_LOG_LEVEL
#define CONFIG_USUM_LOG_LEVEL USUM_LOG_INFO
#endif

void usum_log(int level, const char *fmt, ...);

#define USUM_LOG(level, ...)                  \
    do                                        \
    {                                         \
        if ((level) <= CONFIG_USUM_LOG_LEVEL) \
            usum_log((level), __VA_ARGS__);   \
    } while (0)

#endif // USUM_LOG_H
