#include <common.h>
#include <command.h>
#include <fs.h>
#include <malloc.h>
#include <stdio.h>
#include <console.h>
#include <cli.h>
#include <mapmem.h>

#include "cmd_usum.h"

static int do_usum(struct cmd_tbl_s *cmdtp, int flag, int argc, char *const argv[])
{  
    printf("========== usum ==========\n");
    printf("Welcome to U-Boot System Update Manager (usum)!\n");

	return 0;
}

U_BOOT_CMD(
	usum, 1, 0, do_usum,
	"usum - system update menu for selecting source and update targets",
	"usum - interactively select update source (USB/SD/TFTP) and target files"
);