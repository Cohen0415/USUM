/*
 * Copyright (C) 2025 Cohen0415
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

img_config_t img_from_txt[USUM_IMG_TXT_MAX_IMG_CONFIGS];
int img_from_txt_count = 0;

static img_config_t img_list[USUM_IMG_TXT_MAX_IMG_CONFIGS];
static int img_count = 0;

void img_config_register(const img_config_t *cfg)
{
	if (img_count >= USUM_IMG_TXT_MAX_IMG_CONFIGS)
	{
		return;
	}

	if (!cfg || !cfg->name[0] || !cfg->funs.load || !cfg->funs.check || !cfg->funs.download)
	{
		USUM_LOG(USUM_LOG_ERROR, "Invalid image configuration provided.\n");
		return;
	}

	memcpy(&img_list[img_count++], cfg, sizeof(img_config_t));
}

static void register_all_img_configs(void)
{
	img_uboot_register();
	img_boot_register();
	img_rootfs_register();
}

static int load_file_from_udisk(const storage_configs_t *cfg, const char *filepath, char **buf_out, size_t *len_out)
{
	if (!cfg || !filepath || !buf_out || !len_out)
	{
		USUM_LOG(USUM_LOG_ERROR, "Invalid parameters for loading file from udisk.\n");
		return -1;
	}

	char dev_part[10];
	snprintf(dev_part, sizeof(dev_part), "%s:%s", cfg->stroage_dev_num, cfg->stroage_partition);
	if (fs_set_blk_dev(cfg->stroage_type, dev_part, USUM_FS_TYPE))
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

	if (fs_set_blk_dev(cfg->stroage_type, dev_part, USUM_FS_TYPE))
	{
		USUM_LOG(USUM_LOG_ERROR, "Failed to set blk dev\n");
		return -1;
	}

	loff_t len;
	if (fs_read(filepath, (ulong)buf, 0, file_size, &len))
	{
		USUM_LOG(USUM_LOG_ERROR, "Failed to read %s\n", USUM_IMG_TXT_PATH);
		free(buf);
		return -1;
	}

	buf[len] = '\0';
	*buf_out = buf;
	*len_out = len;
	return 0;
}

static int parse_img_config_buffer(const char *buf, size_t len)
{
	if (!buf || len == 0)
		return -1;

	char *copy = memalign(4, len + 1);
	if (!copy)
		return -1;

	memcpy(copy, buf, len);
	copy[len] = '\0';

	img_from_txt_count = 0;
	char *p = copy, *line;

	while ((line = strsep(&p, "\n")) != NULL)
	{
		while (*line == '\r' || *line == '\n')
			line++;

		if (line[0] == '[')
		{
			if (img_from_txt_count >= USUM_IMG_TXT_MAX_IMG_CONFIGS)
				break;

			char *end = strchr(line, ']');
			if (!end)
				continue;

			*end = '\0';
			strncpy(img_from_txt[img_from_txt_count].name, line + 1, sizeof(img_from_txt[img_from_txt_count].name) - 1);
		}
		else if (strstr(line, "LBA="))
		{
			img_from_txt[img_from_txt_count].addr_start = simple_strtoul(line + 4, NULL, 0);
			img_from_txt_count++;
		}
	}

	for (int i = 0; i < img_from_txt_count; i++)
	{
		img_from_txt[i].size = 0;
		img_from_txt[i].funs.load = NULL;
		img_from_txt[i].funs.check = NULL;
		img_from_txt[i].funs.download = NULL;
	}

	free(copy);
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
				puts("\b \b"); // 删除终端上的字符
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
	if (!dev || !cfg)
	{
		return -1;
	}

	memset(cfg, 0, sizeof(storage_configs_t));

	// init stroage_type
	if (strcmp(dev, USUM_STORAGE_UDISK) == 0)
	{
		strncpy(cfg->stroage_type, "usb", MAX_CFG_LEN - 1);
		run_command("usb reset", 0);
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
	{
		cfg->stroage_type[0] = '\0';
		cfg->stroage_dev_num[0] = '\0';
		cfg->stroage_partition[0] = '\0';
		return -1;
	}

	usum_log(USUM_LOG_INFO, "Source storage configuration:\n");
	usum_log(USUM_LOG_INFO, "- Type: %s\n", cfg->stroage_type);
	usum_log(USUM_LOG_INFO, "- Device Number: %s\n", cfg->stroage_dev_num);
	usum_log(USUM_LOG_INFO, "- Partition: %s\n", cfg->stroage_partition);

	return 0;
}

static int selected_dest_storage_dev(storage_configs_t *cfg)
{
	if (!cfg)
	{
		return -1;
	}

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

	// not use stroage_partition
	cfg->stroage_partition[0] = '\0'; // 清空分区信息

	usum_log(USUM_LOG_INFO, "Destination storage configuration:\n");
	usum_log(USUM_LOG_INFO, "- Type: %s\n", cfg->stroage_type);
	usum_log(USUM_LOG_INFO, "- Device Number: %s\n", cfg->stroage_dev_num);

	return 0;
}

static int selected_img(int selected_id, storage_configs_t *cfg, img_config_t *img)
{
	if (!cfg || !img)
	{
		return -1;
	}

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
	snprintf(dev_part, sizeof(dev_part), "%s:%s", cfg->stroage_dev_num, cfg->stroage_partition);
	if (fs_set_blk_dev(cfg->stroage_type, dev_part, USUM_FS_TYPE))
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

static int download_image(storage_configs_t *cfg, img_config_t *img)
{
	if (!cfg)
	{
		return -1;
	}

	if (!img || img->addr_start == 0)
	{
		usum_log(USUM_LOG_ERROR, "Invalid image configuration.\n");
		return -1;
	}

	char dev_part[10];
	snprintf(dev_part, sizeof(dev_part), "%s:%s", cfg->stroage_dev_num, cfg->stroage_partition);
	if (fs_set_blk_dev(cfg->stroage_type, dev_part, USUM_FS_TYPE))
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
	int ret = img->funs.load(img, cfg, USUM_LOAD_ADDR);
	if (ret < 0)
	{
		USUM_LOG(USUM_LOG_ERROR, "Image load failed for %s\n", img->name);
		return -1;
	}
	USUM_LOG(USUM_LOG_INFO, "Image load successful for %s\n", img->name);

	// 镜像合法性检查
	if (!img->funs.check)
	{
		USUM_LOG(USUM_LOG_ERROR, "Image check function not implemented for %s\n", img->name);
		return -1;
	}
	ret = img->funs.check(img, cfg, (const void *)USUM_LOAD_ADDR);
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
	ret = img->funs.download(img, cfg, USUM_LOAD_ADDR);
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
	storage_configs_t src_storage_cfg;
	storage_configs_t dest_storage_cfg;

	// register your imgs
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

			// 加载配置文件
			char *buf;
			size_t buf_size = 0;
			ret = load_file_from_udisk(&src_storage_cfg, USUM_IMG_TXT_PATH, &buf, &buf_size);
			if (ret < 0)
			{
				usum_log(USUM_LOG_ERROR, "Failed to load image config file %s\n", USUM_IMG_TXT_PATH);
				continue;
			}

			// 解析镜像配置文件
			ret = parse_img_config_buffer(buf, buf_size);
			if (ret < 0)
			{
				usum_log(USUM_LOG_ERROR, "Failed to parse image config file %s\n", USUM_IMG_TXT_PATH);
				continue;
			}

			// 成功选择存储介质，跳出循环
			break;
		}

		// 打印二级菜单（选择要更新的镜像文件）
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

			// 选择镜像文件
			img_config_t sel_img;
			ret = selected_img(selected_id, &src_storage_cfg, &sel_img);
			if (ret < 0)
			{
				usum_log(USUM_LOG_ERROR, "Failed to select image %s\n", img_from_txt[selected_id].name);
				continue;
			}

			// 下载镜像文件
			ret = download_image(&src_storage_cfg, &sel_img);
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
	"usum - interactively select update source (USB/SD/TFTP) and target files");