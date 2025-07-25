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

#ifndef _CMD_USUM_H
#define _CMD_USUM_H

#define USUM_STORAGE_UDISK "udisk"
#define USUM_STORAGE_SDCARD "sdcard"

#define USUM_FS_TYPE FS_TYPE_FAT // 文件系统类型，目前支持FAT

#define USUM_IMG_TXT_PATH "img.txt"           // 镜像配置文件路径
#define USUM_IMG_TXT_MAX_LINE_LEN 128         // 镜像配置文件每行最大长度
#define USUM_IMG_TXT_MAX_IMG_CONFIGS 16       // 镜像配置文件所支持的最大镜像数量
#define USUM_IMG_MAX_SIZE (500 * 1024 * 1024) // 镜像最大大小（500MB）

#define USUM_LOAD_ADDR 0x20000000 // 镜像加载地址（uboot执行bdinfo查看）

#define MAX_CFG_LEN 32
typedef struct storage_configs
{
    char stroage_type[MAX_CFG_LEN];
    char stroage_dev_num[MAX_CFG_LEN];
    char stroage_partition[MAX_CFG_LEN];
} storage_configs_t;

typedef struct img_config img_config_t;
typedef struct img_funs
{
    uint32_t (*load)(img_config_t *img, storage_configs_t *cfg, uint32_t img_addr);
    uint32_t (*check)(img_config_t *img, storage_configs_t *cfg, const void *img_addr);
    uint32_t (*download)(img_config_t *img, storage_configs_t *cfg, uint32_t img_addr);
} img_funs_t;

struct img_config
{
    char name[64];
    uint32_t addr_start;
    uint32_t size;
    img_funs_t funs;
};

void img_config_register(const img_config_t *cfg);

#endif