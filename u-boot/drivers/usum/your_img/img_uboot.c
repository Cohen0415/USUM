#include <common.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
#include <stdio.h>
#include <console.h>
#include <cli.h>
#include <mapmem.h>

#include "../cmd_usum.h"

// <Rockchip_Developer_Guide_UBoot_Nextdev_CN.pdf> - Page 29
#define UBOOT_MAGIC 0xedfe0dd0
static uint32_t check_uboot(img_config_t *img, const void *img_addr) 
{
    printf("checking uboot.img ...\n");

    if (!img_addr) 
    {
        printf("Invalid image address\n");
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

static uint32_t download_uboot(img_config_t *img, uint32_t img_addr) 
{
    if (!img || !img_addr) 
    {
        printf("Invalid image configuration or address\n");
        return -1;
    }

    char cmd[128];
    unsigned long blk_count = (img->size + 511) / 512;  // 向上对齐
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
        .check = check_uboot,
        .download = download_uboot,
    },
};

void img_uboot_register(void)
{
    img_config_register(&img_uboot);
}