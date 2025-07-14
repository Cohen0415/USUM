#ifndef _CMD_USUM_H
#define _CMD_USUM_H

#define USUM_STORAGE_UDISK              "udisk"
#define USUM_STORAGE_SDCARD             "sdcard"

#define USUM_FS_TYPE                    FS_TYPE_FAT     // 文件系统类型，目前支持FAT

#define USUM_IMG_TXT_PATH               "img.txt"       // 镜像配置文件路径
#define USUM_IMG_TXT_MAX_LINE_LEN       128             // 镜像配置文件每行最大长度
#define USUM_IMG_TXT_MAX_IMG_CONFIGS    16              // 镜像配置文件所支持的最大镜像数量

#define USUM_LOAD_ADDR                  0x20000000      // 镜像加载地址（uboot执行bdinfo查看）   

#define MAX_CFG_LEN 32              
typedef struct storage_configs {
    char stroage_type[MAX_CFG_LEN];  
    char stroage_dev_num[MAX_CFG_LEN];
    char stroage_partition[MAX_CFG_LEN];
} storage_configs_t;

typedef struct img_config img_config_t;
typedef struct img_funs {
    uint32_t (*check)(img_config_t *img, const void *img_addr);
    uint32_t (*download)(img_config_t *img, uint32_t img_addr);
} img_funs_t;

struct img_config {
    char name[64];
    uint32_t addr_start;
    uint32_t size;
    img_funs_t funs;
};

void img_config_register(const img_config_t *cfg);

#endif