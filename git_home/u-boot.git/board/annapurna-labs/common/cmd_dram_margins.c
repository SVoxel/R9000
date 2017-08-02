#include <common.h>
#include <command.h>
#include "agent_ddr_margins.h"
#include "al_hal_iomap.h"

int do_dram_margins(
	cmd_tbl_t	*cmdtp,
	int			flag, 
	int 		argc,
	char* const	argv[])
{
	void (*agent)(void) =
			(void (*)(void))CONFIG_AL_SRAM_AGENT_BASE;

	memcpy((void *)CONFIG_AL_SRAM_AGENT_BASE, agent_ddr_margins_arr,
		sizeof(agent_ddr_margins_arr));
	agent();

	/* Should not return */
	return 0;
}


U_BOOT_CMD(
	dram_margins, 1, 0, do_dram_margins,
	"The command provides current DRAM RDQS/WDQS margins by running a shmoo based on the DDR controller BIST feature",
	"margins"
);
