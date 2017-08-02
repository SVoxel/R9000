#ifndef __AL_COMMON_H
#define __AL_COMMON_H

/*
 * High Level Configuration Options
 */
#define CONFIG_ARMCORTEXA15		/* This is an ARM V7 CPU core */

#define CONFIG_SYS_CACHELINE_SIZE	64

#define CONFIG_ARCH_CPU_INIT		/* Fire up the A15 core */

#define CONFIG_DRAM_MAX_MAPPED_ADDR	(3 * (unsigned)SZ_1G)

#define CONFIG_CVOS_TAGS_BASE_ADDR	"0x01000000"
#define CONFIG_CVOS_TAGS_BASE_VALID	"0xcf05cf05"
#define CONFIG_CVOS_TAGS_SEED_A_ADDR	"0x01000004"
#define CONFIG_CVOS_TAGS_SEED_B_ADDR	"0x01000008"

#define CONFIG_AL_BOOTROM_NAND_READ_FUNC_ADDR		0xFFFF1659
#define CONFIG_AL_BOOTROM_NAND_IS_BAD_BLOCK_FUNC_ADDR	0xFFFF1331

#define CONFIG_AL_SRAM_AGENT_BASE	(AL_PBS_INT_MEM_SRAM_BASE + 0x10)
#define CONFIG_AL_CPU_RESUME_BASE	(AL_PBS_INT_MEM_SRAM_BASE + 0x1ec0)

#define CONFIG_CVOS_ENV_COMMANDS					\
	"cvos_tags=" CONFIG_CVOS_TAGS_BASE_ADDR "\0"			\
	"cvos_tags_seed_a=" CONFIG_CVOS_TAGS_SEED_A_ADDR "\0"		\
	"cvos_tags_seed_b=" CONFIG_CVOS_TAGS_SEED_B_ADDR "\0"		\
	"cvos_tags_validate="						\
		"mw.l ${cvos_tags} " CONFIG_CVOS_TAGS_BASE_VALID "\0"	\

#endif /* __AL_COMMON_H */
