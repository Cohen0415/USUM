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

#define CHUNK_SIZE     (100 * 1024 * 1024)  // 每次操作的最大块大小（100MB）   
static uint32_t download_rootfs(img_config_t *img, storage_configs_t *cfg, uint32_t img_addr) 
{
    if (!img || !cfg) 
    {
        printf("Invalid image or storage config\n");
        return -1;
    }

    loff_t total_size = img->size;
    loff_t offset = 0;
    char cmd[256];
    char dev_part[16];

    // 格式如 "mmc 0:1"
    snprintf(dev_part, sizeof(dev_part), "%s %s", cfg->stroage_type, cfg->stroage_dev_num);

    printf("Downloading rootfs: %s, total size: %llu bytes\n", img->name, total_size);

    while (offset < total_size) 
    {
        uint32_t chunk_size = (total_size - offset > CHUNK_SIZE) ? CHUNK_SIZE : (total_size - offset);

        // 避免之前内存残留
        memset((void *)(uintptr_t)img_addr, 0, chunk_size);

        // 1. 加载分段镜像
        snprintf(cmd, sizeof(cmd), "fatload %s 0x%08x %s 0x%x 0x%llx", dev_part, img_addr, img->name, chunk_size, offset);
        if (run_command(cmd, 0)) 
        {
            printf("Failed to load chunk at offset 0x%llx\n", offset);
            return -1;
        }

        // 2. 计算起始 LBA 与块数
        uint32_t blk_count = (chunk_size + 511) / 512;
        uint32_t blk_start = img->addr_start + (offset / 512);

        // 3. 写入 eMMC
        snprintf(cmd, sizeof(cmd), "mmc write 0x%08x 0x%x 0x%x", img_addr, blk_start, blk_count);
        if (run_command(cmd, 0)) 
        {
            printf("Failed to write chunk at LBA 0x%x\n", blk_start);
            return -1;
        }

        // 4. 显示进度
        offset += chunk_size;
        uint32_t percent = (uint32_t)(offset * 100 / total_size);
        printf("[Progress] %3u%% (%llu / %llu bytes)\n", percent, offset, total_size);
    }

    printf("Rootfs download completed successfully.\n");
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