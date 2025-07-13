#include <stdio.h>

#include "../cmd_usum.h"

// <Rockchip_Developer_Guide_UBoot_Nextdev_CN.pdf> - Page 29
#define UBOOT_MAGIC 0xedfe0dd0
static uint32_t check_uboot(const void *img_addr) 
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

static const img_config_t img_uboot = {
    .name = "uboot.img",
    .addr_start = 0,
    .size = 0,
    .funs = {
        .check = check_uboot,
    },
};

void img_uboot_register(void)
{
    img_config_register(&img_uboot);
}