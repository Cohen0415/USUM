#ifndef _CMD_USUM_H
#define _CMD_USUM_H

#define USUM_STORAGE_UDISK              "udisk"
#define USUM_STORAGE_SDCARD             "sdcard"

#define USUM_FS_TYPE                   FS_TYPE_FAT     // 文件系统类型，目前支持FAT

#define MAX_CFG_LEN 32      
typedef struct storage_configs {
    char stroage_type[MAX_CFG_LEN];  
    char stroage_dev_num[MAX_CFG_LEN];
    char stroage_partition[MAX_CFG_LEN];
} storage_configs_t;

#endif