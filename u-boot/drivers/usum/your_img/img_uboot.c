#include <stdio.h>

#include "../cmd_usum.h"
#include "../log_usum.h"

static uint32_t check_uboot(void) 
{
    printf("checking uboot.img ...\n");
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