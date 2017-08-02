#include <common.h>
#include <command.h>

int
do_scripthelp(cmd_tbl_t *cmdtp, int flag, int argc, char* const argv[])
{
	printf("bootupd\t- perform u-boot version update.\n\
		\t* set 'boot_img' to the TFTP server's boot image name.\n");
	printf("delenv\t- delete the env variables of the u-boot img\n");
	return 0;
}


U_BOOT_CMD(
	scripth, 1, 1, do_scripthelp,
	"Show help menu for available scripts",
	"This command will show the available scripts you can run.\n\tRun scripts by using run <SCRIPT_NAME>"
);
