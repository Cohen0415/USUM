#include <stdio.h>

#include "cmd_usum.h"
#include "log_usum.h"

static uint32_t check_boot(void) 
{
    printf("checking boot.img ...\n");
    return 0;
}

static const img_config_t img_boot = {
    .name = "boot.img",
    .addr_start = 0,
    .size = 0,
    .funs = {
        .check = check_boot,
    },
};

void img_boot_register(void)
{
    img_config_register(&img_boot);
}