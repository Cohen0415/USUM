#include <common.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
#include <stdio.h>
#include <console.h>
#include <cli.h>
#include <mapmem.h>

#include "../cmd_usum.h"

static uint32_t load_rootfs(img_config_t *img, storage_configs_t *cfg, uint32_t img_addr)
{
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

#define ROOTFS_MAGIC 0xEF53
static uint32_t check_rootfs(img_config_t *img, storage_configs_t *cfg, const void *img_addr) 
{
    printf("checking rootfs.img (partial read)...\n");

    if (!img || !img->name) 
    {
        printf("Invalid image config or filename\n");
        return -1;
    }

    char dev_part[10];
    snprintf(dev_part, sizeof(dev_part), "%s:%s", cfg->stroage_dev_num, cfg->stroage_partition);
    if (fs_set_blk_dev(cfg->stroage_type, dev_part, USUM_FS_TYPE)) 
    {
        printf("Failed to set blk dev\n");
        return -1;
    }

    // 读取 rootfs.img 中 offset 为 1080 和 1081 的两个字节
    uint8_t header[2];
    loff_t actread = 0;
    if (fs_read(img->name, (ulong)header, 1080, 2, &actread)) 
    {
        printf("Failed to read magic from %s\n", img->name);
        return -1;
    }

    if (actread != 2) 
    {
        printf("Partial read: only %lld bytes read\n", actread);
        return -1;
    }

    uint16_t magic = header[0] | (header[1] << 8);
    if (magic != ROOTFS_MAGIC) 
    {
        printf("Invalid rootfs magic: 0x%04x\n", magic);
        return -1;
    }

    printf("Rootfs image passed magic check\n");
    return 0;
}

#define CHUNK_SIZE     (100 * 1024 * 1024)   // 每次最多处理 100MB 数据
static uint32_t download_rootfs(img_config_t *img, storage_configs_t *cfg, uint32_t img_addr) 
{
    if (!img) 
    {
        printf("Invalid image configuration\n");
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

static const img_config_t img_rootfs = {
    .name = "rootfs.ext2",
    .addr_start = 0,
    .size = 0,
    .funs = {
        .load = load_rootfs,
        .check = check_rootfs,
        .download = download_rootfs,
    },
};

void img_rootfs_register(void)
{
    img_config_register(&img_rootfs);
}