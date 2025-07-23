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

#include <common.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
#include <stdio.h>
#include <console.h>
#include <cli.h>
#include <mapmem.h>

#include "../cmd_usum.h"

static uint32_t load_uboot(img_config_t *img, storage_configs_t *cfg, uint32_t img_addr)
{
    // load mmc 0:1 0x20000000 uboot.img
    char cmd[128];
    char dev_part[10];
    snprintf(dev_part, sizeof(dev_part), "%s:%s", cfg->stroage_dev_num, cfg->stroage_partition);
    snprintf(cmd, sizeof(cmd), "load %s %s 0x%08x %s", cfg->stroage_type, dev_part, img_addr, img->name);

    if (run_command(cmd, 0))
    {
        return -1;
    }

    return 0;
}

// <Rockchip_Developer_Guide_UBoot_Nextdev_CN.pdf> - Page 29
#define UBOOT_MAGIC 0xedfe0dd0
static uint32_t check_uboot(img_config_t *img, storage_configs_t *cfg, const void *img_addr)
{
    printf("checking uboot.img ...\n");

    if (!img || !img_addr)
    {
        printf("Invalid image configuration or address\n");
        return -1;
    }

    if (img->size == 0)
    {
        printf("Image size is zero, cannot check uboot\n");
        return -1;
    }

    if (img->size > 0x1000000)
    {
        printf("%s size exceeds maximum limit (10MB): %u bytes\n", img->name, img->size / (1024 * 1024));
        return -1;
    }

    uint32_t magic = *(uint32_t *)img_addr;
    if (magic != UBOOT_MAGIC)
    {
        printf("Invalid uboot magic: 0x%08x\n", magic);
        return -1;
    }

    return 0;
}

static uint32_t download_uboot(img_config_t *img, storage_configs_t *cfg, uint32_t img_addr)
{
    if (!img)
    {
        printf("Invalid image configuration\n");
        return -1;
    }

    char cmd[128];
    unsigned long blk_count = (img->size + 511) / 512; // 向上对齐
    snprintf(cmd, sizeof(cmd), "mmc write 0x%08x 0x%lx 0x%lx", img_addr, (unsigned long)img->addr_start, blk_count);
    if (run_command(cmd, 0))
    {
        printf("Command failed: %s\n", cmd);
        return -1;
    }

    return 0;
}

static const img_config_t img_uboot = {
    .name = "uboot.img",
    .addr_start = 0,
    .size = 0,
    .funs = {
        .load = load_uboot,
        .check = check_uboot,
        .download = download_uboot,
    },
};

void img_uboot_register(void)
{
    img_config_register(&img_uboot);
}