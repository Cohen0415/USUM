#include <common.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
#include <stdio.h>
#include <console.h>
#include <cli.h>
#include <mapmem.h>

#include "cmd_usum.h"
#include "log_usum.h"

static storage_configs_t cfg;
static void read_line(char *buf, int maxlen)
{
    int i = 0;
    while (i < maxlen - 1) 
	{
        int ch = getc();
        
        if (ch == '\r' || ch == '\n') 
		{
            putc('\n');
            break;
        }

		// 退格键
        if (ch == 0x7F || ch == 0x08) 
		{ 
            if (i > 0) 
			{
                i--;
                puts("\b \b");  // 删除终端上的字符
            }
            continue;
        }

		// 可见字符
        if (ch >= 0x20 && ch <= 0x7E) 
		{ 
            buf[i++] = ch;
            putc(ch);
        }
    }
    buf[i] = '\0';
}

static int selected_storage_dev(char *dev, storage_configs_t *cfg)
{
	memset(cfg, 0, sizeof(storage_configs_t));

	// init stroage_type
	if (strcmp(dev, USUM_STORAGE_UDISK) == 0) 
	{
		strncpy(cfg->stroage_type, "usb", MAX_CFG_LEN - 1);
		run_command("usb start", 0);
	} 
	else if (strcmp(dev, USUM_STORAGE_SDCARD) == 0) 
	{
		strncpy(cfg->stroage_type, "mmc", MAX_CFG_LEN - 1);
	} 
	else 
	{
		usum_log(USUM_LOG_WARN, "Unknown storage device, defaulting to USB.\n");
		strncpy(cfg->stroage_type, "usb", MAX_CFG_LEN - 1);
	}

	char inbuf[10] = {0};

	// init stroage_dev_num
	printf("Enter %s device number (default 0): ", cfg->stroage_type);
	read_line(inbuf, sizeof(inbuf));
	if (inbuf[0] != '\0')
	{
		strncpy(cfg->stroage_dev_num, inbuf, MAX_CFG_LEN - 1);
	}
	else 
	{
		strncpy(cfg->stroage_dev_num, "0", MAX_CFG_LEN - 1);
	}
	
	// init stroage_partition
	printf("Enter %s partition (default 1): ", cfg->stroage_type);
	read_line(inbuf, sizeof(inbuf));
	if (inbuf[0] != '\0')
	{
		strncpy(cfg->stroage_partition, inbuf, MAX_CFG_LEN - 1);
	}
	else 
	{
		strncpy(cfg->stroage_partition, "1", MAX_CFG_LEN - 1);
	}

	// 检查所选存储介质是否存在
	char dev_part[10];
	snprintf(dev_part, sizeof(dev_part), "%s:%s", cfg->stroage_dev_num, cfg->stroage_partition);
	if (fs_set_blk_dev(cfg->stroage_type, dev_part, USUM_FS_TYPE)) 
		return -1;

	usum_log(USUM_LOG_INFO, "Storage configuration:\n");
	usum_log(USUM_LOG_INFO, "- Type: %s\n", cfg->stroage_type);
	usum_log(USUM_LOG_INFO, "- Device Number: %s\n", cfg->stroage_dev_num);	
	usum_log(USUM_LOG_INFO, "- Partition: %s\n", cfg->stroage_partition);

	return 0;
}

static int do_usum(struct cmd_tbl_s *cmdtp, int flag, int argc, char *const argv[])
{  
	char *storage_dev[] = {USUM_STORAGE_UDISK, USUM_STORAGE_SDCARD};
	int storage_dev_count = sizeof(storage_dev) / sizeof(storage_dev[0]);
	char inbuf[16] = {0};
	int ret;

	while (1) 
	{
		// 打印菜单
        printf("\n========== %s ==========\n", "USUM");
        for (int i = 0; i < storage_dev_count; i++) 
		{
            printf("[%d] %s\n", i + 1, storage_dev[i]);
        }
		printf("[q] quit\n");
		printf("Select: ");

		read_line(inbuf, sizeof(inbuf));
		if (inbuf[0] == '\0')
			continue;

		// 退出菜单
		if (inbuf[0] == 'q')    
            break;

		// 输入检查
		int selected_id = inbuf[0] - '1';
		if (selected_id < 0 || selected_id >= storage_dev_count) 
			continue;

		// 存储介质选择
		ret = selected_storage_dev(storage_dev[selected_id], &cfg);
		if (ret < 0)
		{
			usum_log(USUM_LOG_ERROR, "Failed to select storage device %s\n", storage_dev[selected_id]);
			continue;
		}
    }

	return 0;
}

U_BOOT_CMD(
	usum, 1, 0, do_usum,
	"usum - system update menu for selecting source and update targets",
	"usum - interactively select update source (USB/SD/TFTP) and target files"
);