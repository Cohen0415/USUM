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

extern void img_uboot_register(void);
extern void img_boot_register(void);
extern void img_rootfs_register(void);

static storage_configs_t src_storage_cfg;		// 待更新镜像所在的存储介质配置
static storage_configs_t dest_storage_cfg;		// 需要更新的存储介质配置

static img_config_t img_from_txt[USUM_IMG_TXT_MAX_IMG_CONFIGS];
static int img_from_txt_count = 0;

static img_config_t img_list[USUM_IMG_TXT_MAX_IMG_CONFIGS];
static int img_count = 0;
void img_config_register(const img_config_t *cfg)
{
    if (img_count >= USUM_IMG_TXT_MAX_IMG_CONFIGS)
        return;

    memcpy(&img_list[img_count++], cfg, sizeof(img_config_t));
}

static void register_all_img_configs(void)
{
	img_uboot_register();  
	img_boot_register(); 
	img_rootfs_register();
}

static int parse_img_config_file(const char *filepath)
{
    char dev_part[10];
    snprintf(dev_part, sizeof(dev_part), "%s:%s", src_storage_cfg.stroage_dev_num, src_storage_cfg.stroage_partition);
    if (fs_set_blk_dev(src_storage_cfg.stroage_type, dev_part, USUM_FS_TYPE)) 
    {
        USUM_LOG(USUM_LOG_ERROR, "Failed to set blk dev\n");
        return -1;
    }

	loff_t file_size;
    if (fs_size(USUM_IMG_TXT_PATH, &file_size)) 
    {
        USUM_LOG(USUM_LOG_ERROR, "Failed to get size of %s\n", USUM_IMG_TXT_PATH);
        return 0;
    }

    if (file_size == 0)
        return 0;

    if (file_size > 8192) 
    {
        USUM_LOG(USUM_LOG_ERROR, "Invalid file size: %lld\n", file_size);
        return -1;
    }

	char *buf = memalign(4, file_size + 1);
    if (!buf) 
    {
        USUM_LOG(USUM_LOG_ERROR, "Failed to allocate buffer\n");
        return 0;
    }

	if (fs_set_blk_dev(src_storage_cfg.stroage_type, dev_part, USUM_FS_TYPE)) 
    {
        USUM_LOG(USUM_LOG_ERROR, "Failed to set blk dev\n");
        return -1;
    }

    // 读取整个 img.txt 文件
    loff_t len;
    if (fs_read(USUM_IMG_TXT_PATH, (ulong)buf, 0, file_size, &len)) 
    {
        USUM_LOG(USUM_LOG_ERROR, "Failed to read %s\n", USUM_IMG_TXT_PATH);
        free(buf);
        return 0;
    }

    buf[len] = '\0';

	char *p, *line;
    img_from_txt_count = 0;
    p = buf;
    while ((line = strsep(&p, "\n")) != NULL) 
	{
        // 去掉换行符
        while (*line == '\r' || *line == '\n') 
			line++;

        if (line[0] == '[') 
		{
            if (img_from_txt_count >= USUM_IMG_TXT_MAX_IMG_CONFIGS)
                break;

            char *end = strchr(line, ']');
            if (!end) 
				continue;

            *end = '\0';  // 截断 ']'
            strncpy(img_from_txt[img_from_txt_count].name, line + 1, sizeof(img_from_txt[img_from_txt_count].name) - 1);
        } 
		else if (strstr(line, "LBA=")) 
		{
            img_from_txt[img_from_txt_count].addr_start = simple_strtoul(line + 4, NULL, 0);
        } 
		else if (strstr(line, "SIZE=")) 
		{
            img_from_txt[img_from_txt_count].size = simple_strtoul(line + 5, NULL, 0);
            img_from_txt_count++;
        }
    }

    free(buf);
    return 0;
}

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

static int selected_src_storage_dev(char *dev, storage_configs_t *cfg)
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

	usum_log(USUM_LOG_INFO, "Source storage configuration:\n");
	usum_log(USUM_LOG_INFO, "- Type: %s\n", cfg->stroage_type);
	usum_log(USUM_LOG_INFO, "- Device Number: %s\n", cfg->stroage_dev_num);	
	usum_log(USUM_LOG_INFO, "- Partition: %s\n", cfg->stroage_partition);

	return 0;
}

static int selected_dest_storage_dev(storage_configs_t *cfg)
{
	memset(cfg, 0, sizeof(storage_configs_t));

	// init stroage_type
	strncpy(cfg->stroage_type, "mmc", MAX_CFG_LEN - 1);

	char inbuf[10] = {0};

	// init stroage_dev_num
	printf("Enter destination %s device number (default 0): ", cfg->stroage_type);
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
	strncpy(cfg->stroage_partition, "1", MAX_CFG_LEN - 1);

	usum_log(USUM_LOG_INFO, "Destination storage configuration:\n");
	usum_log(USUM_LOG_INFO, "- Type: %s\n", cfg->stroage_type);
	usum_log(USUM_LOG_INFO, "- Device Number: %s\n", cfg->stroage_dev_num);	

	return 0;
}

static int selected_img(int selected_id, img_config_t *img)
{
	if (selected_id < 0 || selected_id >= img_from_txt_count) 
	{
		usum_log(USUM_LOG_ERROR, "Invalid image selection: %d\n", selected_id);
		return -1;
	}

	// 检查镜像名称是否与已注册的镜像配置匹配
	int ret = 1;
	for (int i = 0; i < img_count; i++)
	{
		if (strcmp(img_from_txt[selected_id].name, img_list[i].name) == 0) 
		{
			usum_log(USUM_LOG_INFO, "Image %s is registered.\n", img_from_txt[selected_id].name);
			ret = 0;
			break;
		}
	}
	if (ret) 
	{
		usum_log(USUM_LOG_ERROR, "Image %s is not registered.\n", img_from_txt[selected_id].name);
		return -1;
	}

	// 查看所选镜像是否存在于存储设备中
	char dev_part[10];
	snprintf(dev_part, sizeof(dev_part), "%s:%s", src_storage_cfg.stroage_dev_num, src_storage_cfg.stroage_partition);
	if (fs_set_blk_dev(src_storage_cfg.stroage_type, dev_part, USUM_FS_TYPE)) 
	{
		usum_log(USUM_LOG_ERROR, "Failed to set block device\n");
		return -1;
	}
	loff_t file_size;
	if (fs_size(img_from_txt[selected_id].name, &file_size)) 
	{
		usum_log(USUM_LOG_ERROR, "Can not find image %s in current stroage devices.\n", img_from_txt[selected_id].name);
		return -1;
	}

	// 复制起始地址和文件大小
	img_list[selected_id].addr_start = img_from_txt[selected_id].addr_start;
	img_list[selected_id].size = img_from_txt[selected_id].size;

	// 复制选中的镜像配置
	memcpy(img, &img_list[selected_id], sizeof(img_config_t));
	usum_log(USUM_LOG_INFO, "Selected image: %s\n", img->name);

	return 0;
}

static int download_image(img_config_t *img)
{
	if (!img || img->addr_start == 0) 
	{
		usum_log(USUM_LOG_ERROR, "Invalid image configuration.\n");
		return -1;
	}

	char dev_part[10];
    snprintf(dev_part, sizeof(dev_part), "%s:%s", src_storage_cfg.stroage_dev_num, src_storage_cfg.stroage_partition);
    if (fs_set_blk_dev(src_storage_cfg.stroage_type, dev_part, USUM_FS_TYPE)) 
    {
        USUM_LOG(USUM_LOG_ERROR, "Failed to set blk dev\n");
        return -1;
    }

	loff_t file_size;
    if (fs_size(img->name, &file_size)) 
    {
        USUM_LOG(USUM_LOG_ERROR, "Failed to get size of %s\n", img->name);
        return -1;
    }
    if (file_size == 0)
	{
        return -1;
	}
	img->size = file_size;
	USUM_LOG(USUM_LOG_INFO, "%s size: %lld Mbytes\n", img->name, img->size / (1024 * 1024));

	// 加载镜像到指定内存
	if (!img->funs.load) 
	{
		USUM_LOG(USUM_LOG_ERROR, "Image load function not implemented for %s\n", img->name);
		return -1;
	}
	int ret = img->funs.load(img, &src_storage_cfg, USUM_LOAD_ADDR);
	if (ret < 0)
	{
		USUM_LOG(USUM_LOG_ERROR, "Image load failed for %s\n", img->name);
		return -1;
	}
	USUM_LOG(USUM_LOG_INFO, "Image load successful for %s\n", img->name);

	// 用户自定义镜像的fops中的check函数，检查待更新镜像是否有效
	if (!img->funs.check) 
	{
		USUM_LOG(USUM_LOG_ERROR, "Image check function not implemented for %s\n", img->name);
		return -1;
	}
	ret = img->funs.check(img, &src_storage_cfg, (const void *)USUM_LOAD_ADDR);
    if (ret < 0) 
	{
        USUM_LOG(USUM_LOG_ERROR, "Image check failed for %s\n", img->name);
        return -1;
    }
	USUM_LOG(USUM_LOG_INFO, "Image check passed for %s\n", img->name);

	// 下载镜像
	if (!img->funs.download) 
	{
		USUM_LOG(USUM_LOG_ERROR, "Image download function not implemented for %s\n", img->name);
		return -1;
	}
	ret = img->funs.download(img, &src_storage_cfg, USUM_LOAD_ADDR);
	if (ret < 0) 
	{
		USUM_LOG(USUM_LOG_ERROR, "Image download failed for %s\n", img->name);
		return -1;
	}
	USUM_LOG(USUM_LOG_INFO, "Image download successful for %s\n", img->name);

	return 0;
}

static int do_usum(struct cmd_tbl_s *cmdtp, int flag, int argc, char *const argv[])
{  
	char *storage_dev[] = {USUM_STORAGE_UDISK, USUM_STORAGE_SDCARD};
	int storage_dev_count = sizeof(storage_dev) / sizeof(storage_dev[0]);
	char inbuf[16] = {0};
	int ret;
	int selected_id;

	register_all_img_configs();

	while (1)
	{	
		// 打印一级菜单（存储介质选择）
		while (1) 
		{
			printf("\n========== %s ==========\n", "USUM");
			for (int i = 0; i < storage_dev_count; i++) 
			{
				printf("[%d] %s\n", i + 1, storage_dev[i]);
			}
			printf("[r] reboot: restart the system\n");
            printf("[q] quit:   quit the menu\n");
			printf("Select: ");

			read_line(inbuf, sizeof(inbuf));
			if (inbuf[0] == '\0')
				continue;
			
			// 重启系统
			if (inbuf[0] == 'r')    
            {
                run_command("reboot", 0);
                return 0;
            }

			// 退出菜单
			if (inbuf[0] == 'q')    
				return 0;

			// 输入检查
			selected_id = inbuf[0] - '1';
			if (selected_id < 0 || selected_id >= storage_dev_count) 
				continue;

			// 待更新镜像所在的存储设备选择
			ret = selected_src_storage_dev(storage_dev[selected_id], &src_storage_cfg);
			if (ret < 0)
			{
				usum_log(USUM_LOG_ERROR, "Failed to select storage device %s\n", storage_dev[selected_id]);
				continue;
			}

			// 需要更新的存储介质设备选择
			printf("\n");
			ret = selected_dest_storage_dev(&dest_storage_cfg);
			if (ret < 0)
			{
				usum_log(USUM_LOG_ERROR, "Failed to select destination storage device\n");
				continue;
			}

			// 解析镜像配置文件
			ret = parse_img_config_file(USUM_IMG_TXT_PATH);
			if (ret < 0)
			{
				usum_log(USUM_LOG_ERROR, "Failed to parse image config file %s\n", USUM_IMG_TXT_PATH);
				continue;
			}

			break;  // 成功选择存储介质，跳出循环
		}

		// 打印二级菜单（可选择更新的镜像文件）
		while (1)
		{
			printf("\n========== %s ==========\n", "Imgs");
			for (int i = 0; i < img_from_txt_count; ++i) 
			{
				printf("[%d] %s\t\t(LBA=0x%08x)\n",
					i + 1,
					img_from_txt[i].name,
					img_from_txt[i].addr_start);
			}
			printf("[r] reboot: restart the system\n");
			printf("[b] back:   return to previous menu\n");
			printf("Select: ");

			read_line(inbuf, sizeof(inbuf));
			if (inbuf[0] == '\0')
				continue;

			// 重启系统
			if (inbuf[0] == 'r')    
            {
                run_command("reboot", 0);
                return 0;
            }

			// 返回上级菜单
			if (inbuf[0] == 'b')    
				break;

			// 输入检查
			selected_id = inbuf[0] - '1';
			if (selected_id < 0 || selected_id >= img_from_txt_count) 
				continue;

			// 选择镜像文件进行更新
			img_config_t sel_img;
			ret = selected_img(selected_id, &sel_img);
			if (ret < 0)
			{
				usum_log(USUM_LOG_ERROR, "Failed to select image %s\n", img_from_txt[selected_id].name);
				continue;
			}

			// 镜像下载
			ret = download_image(&sel_img);
			if (ret < 0)
			{
				usum_log(USUM_LOG_ERROR, "Failed to download image %s\n", img_from_txt[selected_id].name);
				continue;
			}
			usum_log(USUM_LOG_INFO, "Image %s downloaded successfully.\n", sel_img.name);
		}
	}

	return 0;
}

U_BOOT_CMD(
	usum, 1, 0, do_usum,
	"usum - system update menu for selecting source and update targets",
	"usum - interactively select update source (USB/SD/TFTP) and target files"
);