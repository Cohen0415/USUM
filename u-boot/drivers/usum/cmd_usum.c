#include <common.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
#include <stdio.h>
#include <console.h>
#include <cli.h>
#include <mapmem.h>

#include "cmd_usum.h"

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

static void selected_storage_dev(char *dev)
{
	// 这里可以添加对选中设备的处理逻辑
	printf("Selected storage device: %s\n", dev);
}

static int do_usum(struct cmd_tbl_s *cmdtp, int flag, int argc, char *const argv[])
{  
	char *storage_dev[] = {"udisk", "sd card", "tftp"};
	int storage_dev_count = sizeof(storage_dev) / sizeof(storage_dev[0]);
	char inbuf[16] = {0};

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

		// 设备选择
		selected_storage_dev(storage_dev[selected_id]);
    }

	return 0;
}

U_BOOT_CMD(
	usum, 1, 0, do_usum,
	"usum - system update menu for selecting source and update targets",
	"usum - interactively select update source (USB/SD/TFTP) and target files"
);