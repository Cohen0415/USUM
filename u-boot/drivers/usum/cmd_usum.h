#ifndef _CMD_USUM_H
#define _CMD_USUM_H

#define USUM_STORAGE_UDISK              "udisk"
#define USUM_STORAGE_SDCARD             "sdcard"

#define USUM_FS_TYPE                    FS_TYPE_FAT     // 文件系统类型，目前支持FAT

#define USUM_IMG_TXT_PATH               "img.txt"       // 镜像配置文件路径
#define USUM_IMG_TXT_MAX_LINE_LEN       128             // 镜像配置文件每行最大长度
#define USUM_IMG_TXT_MAX_IMG_CONFIGS    16              // 镜像配置文件所支持的最大镜像数量

#define MAX_CFG_LEN 32              
typedef struct storage_configs {
    char stroage_type[MAX_CFG_LEN];  
    char stroage_dev_num[MAX_CFG_LEN];
    char stroage_partition[MAX_CFG_LEN];
} storage_configs_t;

typedef struct img_funs {
    uint32_t (*compatible)(void);
    uint32_t (*load)(void);
    uint32_t (*check)(void);
    uint32_t (*download)(void);
} img_funs_t;

typedef struct img_config {
    char name[64];                      // 镜像名称，如 "boot.img"
    uint32_t addr_start;                // 起始 LBA
    uint32_t size;                      // 文件大小
    img_funs_t funs;                    // 镜像操作函数
} img_config_t;

#endif