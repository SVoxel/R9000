#include <common.h>
#include <command.h>

int
do_confirm_msg(cmd_tbl_t *cmdtp, int flag, int argc, char* const argv[])
{
	int user_input;

	if( argc == 2)
		printf(argv[1]);
	else
		printf("[y/n]?");
	user_input = getc();
	printf("%c\n", user_input);
	return !(user_input == 'y' || user_input == 'Y');
}


U_BOOT_CMD(
	confirm_msg, 2, 0, do_confirm_msg,
	"get user input and updates \"?\" varialbe to 0 if 'y' was entered",
	"[confirm_message]"
);
