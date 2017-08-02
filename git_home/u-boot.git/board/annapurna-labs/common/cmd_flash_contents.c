#include <common.h>
#include <command.h>
#include <malloc.h>
#include <spi_flash.h>
#include <nand.h>
#include <al_globals.h>

#include "al_flash_contents.h"

#define ROUND_DOWN(a,b)		((a) & ~((b) - 1))

#define MAX_PAGE_SIZE		SZ_8K

typedef int (*al_flash_dev_erase)(unsigned int, unsigned int);
typedef int (*al_flash_dev_write)(unsigned int, void *, unsigned int);

static unsigned int toc_offset;
static al_flash_dev_read dev_read = NULL;
static al_flash_dev_erase dev_erase = NULL;
static al_flash_dev_write dev_write = NULL;
static unsigned int dev_erase_size;
static struct spi_flash *spi_flash = NULL;
static nand_info_t *nand_flash = NULL;
static uint8_t nand_page[MAX_PAGE_SIZE];

static unsigned int obj_id_from_args(const char *arg_id, const char *arg_instance)
{
	int id;

	id = al_flash_obj_id_from_str(arg_id);
	if (id >= 0)
		id = id;
	else
		id = simple_strtoul(arg_id, NULL, 16);

	return AL_FLASH_OBJ_ID(id, simple_strtoul(arg_instance, NULL, 16));
}

static int flash_contents_mem_read(unsigned int offset, void *buff, unsigned int size)
{
	memcpy(buff, (const void *)(uintptr_t)offset, size);

	return 0;
}

static int flash_contents_nand_read(unsigned int offset, void *buff, unsigned int size)
{
	int err;
	unsigned int page_size = nand_flash->writesize;
	unsigned int round_offset =
		ROUND_DOWN(offset, page_size);
	unsigned int before_offset = offset - round_offset;
	size_t rwsize;

	debug("%s(%08x, %p, %08x)\n", __func__, offset, buff, size);

	if (before_offset) {
		unsigned int curr_size =
			((size + before_offset) > page_size) ?
			page_size : size + before_offset;
		unsigned int num_useful_bytes = curr_size - before_offset;

		rwsize = curr_size;
		debug("\tnand_read_skip_bad(%08x, %p, %08x)\n", round_offset, nand_page, (unsigned int)rwsize);
		err = nand_read_skip_bad(nand_flash, round_offset, &rwsize, NULL, nand_flash->size, nand_page);
		if (err)
			return err;

		memcpy(buff, &nand_page[before_offset], num_useful_bytes);
		buff += num_useful_bytes;
		size -= num_useful_bytes;
		offset += num_useful_bytes;
	}

	if (size) {
		rwsize = size;
		debug("\tnand_read_skip_bad(%08x, %p, %08x)\n", offset, buff, (unsigned int)rwsize);
		err = nand_read_skip_bad(nand_flash, offset, &rwsize, NULL, nand_flash->size, buff);
		if (err)
			return err;
	}

	return 0;
}

static int flash_contents_nand_erase(unsigned int offset, unsigned int size)
{
	nand_erase_options_t opts;

	debug("%s(%08x, %08x)\n", __func__, offset, size);

	memset(&opts, 0, sizeof(opts));
	opts.offset = offset;
	opts.length = size;
	opts.spread = 1;
	opts.quiet = 1;

	return nand_erase_opts(nand_flash, &opts);
}

static int flash_contents_nand_write(unsigned int offset, void *buff, unsigned int size)
{
	size_t rwsize = size;

	debug("%s(%08x, %p, %08x)\n", __func__, offset, buff, size);

	return nand_write_skip_bad(nand_flash, offset, &rwsize, NULL, nand_flash->size, buff, 0);
}

static int flash_contents_spi_read(unsigned int offset, void *buff, unsigned int size)
{
	int err;

	debug("%s(%08x, %p, %08x)\n", __func__, offset, buff, size);

	err = spi_flash_read(spi_flash, offset, size, buff);
	if (err)
		return err;

	return 0;
}

static int flash_contents_spi_erase(unsigned int offset, unsigned int size)
{
	int err;

	debug("%s(%08x, %08x)\n", __func__, offset, size);

	err = spi_flash_erase(spi_flash, offset, size);
	if (err)
		return err;

	return 0;
}

static int flash_contents_spi_write(unsigned int offset, void *buff, unsigned int size)
{
	int err;

	debug("%s(%08x, %p, %08x)\n", __func__, offset, buff, size);

	err = spi_flash_write(spi_flash, offset, size, buff);
	if (err)
		return err;

	return 0;
}

static void flash_contents_set_dev(enum boot_device boot_device)
{
	if (boot_device == BOOT_DEVICE_NAND_8BIT) {
		if (!nand_flash)
			nand_flash = &nand_info[0];

		dev_read = flash_contents_nand_read;
		dev_erase = flash_contents_nand_erase;
		dev_write = flash_contents_nand_write;
		dev_erase_size = nand_flash->erasesize;

	} else {
		if (!spi_flash)
			spi_flash = spi_flash_probe(0, 0, CONFIG_SF_DEFAULT_SPEED,
				CONFIG_SF_DEFAULT_MODE);

		dev_read = flash_contents_spi_read;
		dev_erase = flash_contents_spi_erase;
		dev_write = flash_contents_spi_write;
		dev_erase_size = spi_flash->erase_size;
	}
}

static int flash_contents_get_dev(void)
{
	int err;

	if (!dev_read)
		flash_contents_set_dev(al_globals.bootstraps.boot_device);

	err = al_flash_toc_search(dev_read,
		CONFIG_AL_FLASH_TOC_FIRST_OFFSET,
		CONFIG_AL_FLASH_TOC_SKIP_SIZE,
		CONFIG_AL_FLASH_TOC_MAX_NUM_SKIPS,
		&toc_offset);
	if (err) {
		printf("al_flash_toc_search failed!\n");
		return err;
	}

	return 0;
}

static int do_flash_contents_set_dev(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc != 2)
		return -1;

	if (!strcmp(argv[1], "boot"))
		flash_contents_set_dev(al_globals.bootstraps.boot_device);
	else if (!strcmp(argv[1], "spi"))
		flash_contents_set_dev(BOOT_DEVICE_SPI_MODE_3);
	else if (!strcmp(argv[1], "nand"))
		flash_contents_set_dev(BOOT_DEVICE_NAND_8BIT);
	else
		return -1;

	return 0;
}

static int do_flash_contents_toc_print(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int err;

	err = flash_contents_get_dev();
	if (err)
		return 1;

	al_flash_toc_print(dev_read, toc_offset);

	return 0;
}

static int do_flash_contents_obj_info_print(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int err;
	int found_index;
	struct al_flash_toc_entry toc_entry;
	unsigned int obj_id;
	void *temp_buff;

	if (argc != 3)
		return -1;

	obj_id = obj_id_from_args(argv[1], argv[2]);

	err = flash_contents_get_dev();
	if (err) {
		err = 1;
		goto done;
	}

	temp_buff = malloc(dev_erase_size);
	if (!temp_buff) {
		err = 1;
		goto done;
	}

	err = al_flash_toc_find_id(dev_read, toc_offset, obj_id, 0,
		&found_index, &toc_entry);
	if (err) {
		printf("al_flash_toc_find_id failed!\n");
		err = 1;
		goto done_free;
	}
	if (found_index < 0) {
		printf("Object not found in TOC!\n");
		err = 1;
		goto done_free;
	}

	err = al_flash_obj_info_print(dev_read, toc_entry.offset, toc_entry.obj_id,
			temp_buff, dev_erase_size);
	if (err) {
		printf("al_flash_obj_info_print failed!\n");
		err = 1;
		goto done_free;
	}

done_free:
	free(temp_buff);
done:
	return err;
}

static int flash_contents_obj_read(
	unsigned int			obj_id,
	struct al_flash_obj_hdr		*obj_hdr,
	void				*data,
	unsigned int			max_size)
{
	int err;
	int found_index;
	struct al_flash_toc_entry toc_entry;

	err = flash_contents_get_dev();
	if (err)
		return err;

	err = al_flash_toc_find_id(dev_read, toc_offset, obj_id, 0, &found_index, &toc_entry);
	if (err) {
		printf("al_flash_toc_find_id failed!\n");
		return err;
	}
	if (found_index < 0) {
		printf("Object not found in TOC!\n");
		return -ENODEV;
	}

	err = al_flash_obj_header_read_and_validate(dev_read, toc_entry.offset,	obj_hdr);
	if (err) {
		printf("al_flash_obj_header_read_and_validate failed!\n");
		return err;
	}

	if (max_size < obj_hdr->size) {
		printf("Object too big!\n");
		return -ENOMEM;
	}

	err = al_flash_obj_data_load(dev_read, toc_entry.offset, data);
	if (err) {
		printf("al_flash_obj_data_load failed!\n");
		return err;
	}

	return 0;
}

static int do_flash_contents_obj_read(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int err;
	unsigned int obj_id;
	unsigned int load_addr;
	struct al_flash_obj_hdr obj_hdr;

	if (argc < 3)
		return -1;

	obj_id = obj_id_from_args(argv[1], argv[2]);

	if (argc > 3)
		load_addr = simple_strtoul(argv[3], NULL, 16);
	else
		load_addr = getenv_ulong("loadaddr", 16, CONFIG_LOADADDR);

	err = flash_contents_obj_read(obj_id, &obj_hdr, (void *)(uintptr_t)load_addr, 0xffffffff);
	if (err) {
		printf("flash_contents_obj_read failed!\n");
		return 1;
	}

	return 0;
}

static int do_flash_contents_obj_read_mem(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int err;
	struct al_flash_obj_hdr obj_hdr;
	unsigned int read_addr;
	unsigned int load_addr;

	if (argc < 3)
		return -1;

	load_addr = simple_strtoul(argv[1], NULL, 16);

	if (argc > 2)
		read_addr = simple_strtoul(argv[2], NULL, 16);
	else
		read_addr = getenv_ulong("loadaddr", 16, CONFIG_LOADADDR);

	err = al_flash_obj_header_read_and_validate(flash_contents_mem_read,
		read_addr, &obj_hdr);
	if (err) {
		printf("al_flash_obj_header_read_and_validate failed!\n");
		return 1;
	}

	err = al_flash_obj_data_load(flash_contents_mem_read, read_addr, (void *)(uintptr_t)load_addr);
	if (err) {
		printf("al_flash_obj_data_load failed!\n");
		return 1;
	}

	return 0;
}

static int do_flash_contents_obj_validate(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int err;
	int found_index;
	struct al_flash_toc_entry toc_entry;
	struct al_flash_obj_hdr obj_hdr;
	unsigned int obj_id;
	unsigned int load_addr;
	unsigned int size;

	if (argc < 3)
		return -1;

	obj_id = obj_id_from_args(argv[1], argv[2]);

	if (argc > 3)
		load_addr = simple_strtoul(argv[3], NULL, 16);
	else
		load_addr = getenv_ulong("loadaddr", 16, CONFIG_LOADADDR);

	if (argc > 4)
		size = simple_strtoul(argv[4], NULL, 16);
	else
		size = SZ_1M;

	err = flash_contents_get_dev();
	if (err)
		return 1;

	err = al_flash_toc_find_id(dev_read, toc_offset, obj_id, 0,
		&found_index, &toc_entry);
	if (err) {
		printf("al_flash_toc_find_id failed!\n");
		return 1;
	}
	if (found_index < 0) {
		printf("Object not found in TOC!\n");
		return 1;
	}

	if (AL_FLASH_OBJ_ID_ID(obj_id) == AL_FLASH_OBJ_ID_STG2) {
		err = al_flash_stage2_validate(dev_read, toc_entry.offset, (void *)(uintptr_t)load_addr, size, &obj_hdr);
		if (err) {
			printf("al_flash_stage2_validate failed!\n");
			return 1;
		}
	} else if (AL_FLASH_OBJ_ID_ID(obj_id) == AL_FLASH_OBJ_ID_PRE_BOOT) {
		err = al_flash_pre_boot_validate(dev_read, toc_entry.offset, (void *)(uintptr_t)load_addr, size, &obj_hdr);
		if (err) {
			printf("al_flash_pre_boot_validate failed!\n");
			return 1;
		}
	} else {
		err = al_flash_obj_validate(dev_read, toc_entry.offset, (void *)(uintptr_t)load_addr, size, &obj_hdr);
		if (err) {
			printf("al_flash_obj_validate failed!\n");
			return 1;
		}
	}

	return 0;
}

static int flash_contents_obj_update(unsigned int obj_id, void *obj_buff, unsigned int size)
{
	int err;
	int found_index;
	struct al_flash_toc_entry toc_entry;

	unsigned int first_block_addr;
	unsigned int first_block_num_bytes;
	void *first_block_buff;
	unsigned int last_block_addr;
	unsigned int last_block_num_bytes;
	void *last_block_buff;

	err = flash_contents_get_dev();
	if (err) {
		goto done;
	}

	first_block_buff = malloc(dev_erase_size);
	if (!first_block_buff) {
		printf("Unable to allocate first block buff!\n");
		err = -ENOMEM;
		goto done;
	}

	last_block_buff = malloc(dev_erase_size);
	if (!last_block_buff) {
		printf("Unable to allocate last block buff!\n");
		err = -ENOMEM;
		goto done_free_first;
	}

	err = al_flash_toc_find_id(dev_read, toc_offset, obj_id, 0,
		&found_index, &toc_entry);
	if (err) {
		printf("al_flash_toc_find_id failed!\n");
		goto done_free_last;
	}
	if (found_index < 0) {
		printf("Object not found in TOC!\n");
		err = -ENODEV;
		goto done_free_last;
	}

	if (size > toc_entry.max_size) {
		printf("Object exceed maximal size of %u bytes!\n", toc_entry.max_size);
		err = -ENOMEM;
		goto done_free_last;
	}

	first_block_addr = ROUND_DOWN(toc_entry.offset, dev_erase_size);
	first_block_num_bytes = (toc_entry.offset - first_block_addr);
	if (first_block_num_bytes) {
		err = dev_read(first_block_addr, first_block_buff, dev_erase_size);
		if (err) {
			printf("dev_read failed!\n");
			goto done_free_last;
		}
	}

	last_block_addr = ROUND_DOWN(toc_entry.offset + size - 1, dev_erase_size);
	last_block_num_bytes = (last_block_addr + dev_erase_size) - (toc_entry.offset + size);
	if (last_block_num_bytes) {
		err = dev_read(last_block_addr, last_block_buff, dev_erase_size);
		if (err) {
			printf("dev_read failed!\n");
			goto done_free_last;
		}
	}

	err = dev_erase(
		first_block_addr,
		(last_block_addr + dev_erase_size) - first_block_addr);
	if (err) {
		printf("dev_erase failed!\n");
		goto done_free_last;
	}

	if (first_block_num_bytes) {
		unsigned int copy_size =
			(size > (dev_erase_size - first_block_num_bytes)) ?
			(dev_erase_size - first_block_num_bytes) :
			size;

		debug("memcpy(%p, %p, %08x)\n", first_block_buff + first_block_num_bytes, obj_buff, copy_size);
		memcpy(first_block_buff + first_block_num_bytes, obj_buff, copy_size);
		obj_buff += copy_size;
		size -= copy_size;
		toc_entry.offset += copy_size;
		err = dev_write(first_block_addr, first_block_buff, dev_erase_size);
		if (err) {
			printf("dev_write failed!\n");
			goto done_free_last;
		}
	}

	if (size > dev_erase_size) {
		unsigned int size_current = ROUND_DOWN(size, dev_erase_size);

		err = dev_write(toc_entry.offset, obj_buff, size_current);
		if (err) {
			printf("dev_write failed!\n");
			goto done_free_last;
		}

		obj_buff += size_current;
		size -= size_current;
		toc_entry.offset += size_current;
	}

	if (last_block_num_bytes && ((last_block_addr > first_block_addr) || (!first_block_num_bytes))) {
		debug("memcpy(%p, %p, %08x)\n", last_block_buff, obj_buff, size);
		memcpy(last_block_buff, obj_buff, size);

		/**
		 * Stage 2 first byte should be written last as it might cause active
		 * image change
		 */
		if ((dev_write != flash_contents_nand_write) &&
			((AL_FLASH_OBJ_ID_ID(obj_id) == AL_FLASH_OBJ_ID_STG2) ||
			(AL_FLASH_OBJ_ID_ID(obj_id) == AL_FLASH_OBJ_ID_PRE_BOOT))) {
			err = dev_write(last_block_addr + 1, last_block_buff + 1, dev_erase_size - 1);
			if (err) {
				printf("dev_write failed!\n");
				goto done_free_last;
			}

			err = dev_write(last_block_addr, last_block_buff, 1);
			if (err) {
				printf("dev_write failed!\n");
				goto done_free_last;
			}
		} else {
			err = dev_write(last_block_addr, last_block_buff, dev_erase_size);
			if (err) {
				printf("dev_write failed!\n");
				goto done_free_last;
			}
		}
	}

done_free_last:
	free(last_block_buff);
done_free_first:
	free(first_block_buff);
done:
	return err;
}

static int do_flash_contents_obj_update(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int err;
	unsigned int obj_id;
	unsigned int load_addr;
	unsigned int size;

	if (argc < 3)
		return -1;

	obj_id = obj_id_from_args(argv[1], argv[2]);

	if (argc > 3)
		load_addr = simple_strtoul(argv[3], NULL, 16);
	else
		load_addr = getenv_ulong("loadaddr", 16, CONFIG_LOADADDR);

	if (argc > 4)
		size = simple_strtoul(argv[4], NULL, 16);
	else
		size = getenv_ulong("filesize", 16, 0);

	err = flash_contents_obj_update(obj_id, (void *)(uintptr_t)load_addr, size);
	if (err)
		return 1;

	return 0;
}

static int do_flash_contents_instance_invalidate(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int err;
	int found_index;
	struct al_flash_toc_entry toc_entry;
	unsigned int obj_id;
	unsigned int block_addr;
	void *block_buff;

	if (argc != 2)
		return -1;

	err = flash_contents_get_dev();
	if (err)
		return 1;

	block_buff = malloc(dev_erase_size);
	if (!block_buff) {
		printf("Unable to allocate block buff!\n");
		return 1;
	}

	obj_id = AL_FLASH_OBJ_ID(
		AL_FLASH_OBJ_ID_PRE_BOOT,
		simple_strtoul(argv[1], NULL, 16));

	err = al_flash_toc_find_id(dev_read, toc_offset, obj_id, 0,
		&found_index, &toc_entry);
	if (err) {
		printf("al_flash_toc_find_id failed!\n");
		goto try_stage2;
	}
	if (found_index < 0) {
		printf("Object not found in TOC!\n");
		goto try_stage2;
	}

	goto found;

try_stage2:

	obj_id = AL_FLASH_OBJ_ID(
		AL_FLASH_OBJ_ID_STG2,
		simple_strtoul(argv[1], NULL, 16));

	err = al_flash_toc_find_id(dev_read, toc_offset, obj_id, 0,
		&found_index, &toc_entry);
	if (err) {
		printf("al_flash_toc_find_id failed!\n");
		err = 1;
		goto done_free;
	}
	if (found_index < 0) {
		printf("Object not found in TOC!\n");
		err = 1;
		goto done_free;
	}

found:

	block_addr = ROUND_DOWN(toc_entry.offset, dev_erase_size);
	err = dev_read(block_addr, block_buff, dev_erase_size);
	if (err) {
		printf("dev_read failed!\n");
		err = 1;
		goto done_free;
	}

	((uint8_t *)block_buff)[toc_entry.offset - block_addr] = 0;

	err = dev_erase(block_addr, dev_erase_size);
	if (err) {
		printf("dev_erase failed!\n");
		err = 1;
		goto done_free;
	}

	err = dev_write(block_addr, block_buff, dev_erase_size);
	if (err) {
		printf("dev_write failed!\n");
		err = 1;
		goto done_free;
	}

done_free:
	free(block_buff);

	return err;
}

static int do_flash_contents_instance_revalidate(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int err;
	int found_index;
	struct al_flash_toc_entry toc_entry;
	unsigned int obj_id;
	unsigned int block_addr;
	void *block_buff;

	if (argc != 2)
		return -1;

	err = flash_contents_get_dev();
	if (err)
		return 1;

	block_buff = malloc(dev_erase_size);
	if (!block_buff) {
		printf("Unable to allocate block buff!\n");
		return 1;
	}

	obj_id = AL_FLASH_OBJ_ID(
		AL_FLASH_OBJ_ID_PRE_BOOT,
		simple_strtoul(argv[1], NULL, 16));

	err = al_flash_toc_find_id(dev_read, toc_offset, obj_id, 0,
		&found_index, &toc_entry);
	if (err) {
		printf("al_flash_toc_find_id failed!\n");
		goto try_stage2;
	}
	if (found_index < 0) {
		printf("Object not found in TOC!\n");
		goto try_stage2;
	}

	goto found;

try_stage2:

	obj_id = AL_FLASH_OBJ_ID(
		AL_FLASH_OBJ_ID_STG2,
		simple_strtoul(argv[1], NULL, 16));

	err = al_flash_toc_find_id(dev_read, toc_offset, obj_id, 0,
		&found_index, &toc_entry);
	if (err) {
		printf("al_flash_toc_find_id failed!\n");
		err = 1;
		goto done_free;
	}
	if (found_index < 0) {
		printf("Object not found in TOC!\n");
		err = 1;
		goto done_free;
	}

found:

	block_addr = ROUND_DOWN(toc_entry.offset, dev_erase_size);
	err = dev_read(block_addr, block_buff, dev_erase_size);
	if (err) {
		printf("dev_read failed!\n");
		err = 1;
		goto done_free;
	}

	((uint8_t *)block_buff)[toc_entry.offset - block_addr] = 'S';

	err = dev_erase(block_addr, dev_erase_size);
	if (err) {
		printf("dev_erase failed!\n");
		err = 1;
		goto done_free;
	}

	err = dev_write(block_addr, block_buff, dev_erase_size);
	if (err) {
		printf("dev_write failed!\n");
		err = 1;
		goto done_free;
	}

done_free:
	free(block_buff);

	return err;
}

static int do_flash_contents_boot_mode_default_app_set(
	cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	struct boot_mode_obj {
		struct al_flash_obj_hdr		header;
		struct al_flash_boot_mode	boot_mode;
		uint32_t			data_checksum;
	} boot_mode_obj;
	unsigned int obj_id;
	int err;
	int i;

	assert(offsetof(struct boot_mode_obj, boot_mode) == sizeof(struct al_flash_obj_hdr));

	assert(offsetof(struct boot_mode_obj, data_checksum) ==
		(sizeof(struct al_flash_obj_hdr) + sizeof(struct al_flash_obj_hdr)));

	if (argc < 2)
		return -1;

	obj_id = AL_FLASH_OBJ_ID(AL_FLASH_OBJ_ID_BOOT_MODE, 0);
	err = flash_contents_obj_read(obj_id, &boot_mode_obj.header, &boot_mode_obj.boot_mode,
		sizeof(struct al_flash_boot_mode));
	if (err) {
		printf("flash_contents_obj_read failed!\n");
		return 1;
	}

	boot_mode_obj.boot_mode.default_app_id = obj_id_from_args(argv[1], "0");

	for (i = 0, boot_mode_obj.data_checksum = 0; i < boot_mode_obj.header.size; i++)
		boot_mode_obj.data_checksum += ((uint8_t *)&boot_mode_obj.boot_mode)[i];

	err = flash_contents_obj_update(obj_id, &boot_mode_obj, sizeof(struct boot_mode_obj));
	if (err) {
		printf("flash_contents_obj_update failed!\n");
		return 1;
	}

	return 0;
}

static int do_flash_contents_boot_mode_default_app_show(
	cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	struct boot_mode_obj {
		struct al_flash_obj_hdr		header;
		struct al_flash_boot_mode	boot_mode;
		uint32_t			data_checksum;
	} boot_mode_obj;
	unsigned int obj_id;
	int err;

	obj_id = AL_FLASH_OBJ_ID(AL_FLASH_OBJ_ID_BOOT_MODE, 0);
	err = flash_contents_obj_read(obj_id, &boot_mode_obj.header, &boot_mode_obj.boot_mode,
		sizeof(struct al_flash_boot_mode));
	if (err) {
		printf("flash_contents_obj_read failed!\n");
		return 1;
	}

	printf("Default app: 0x%x (%s)\n",
		boot_mode_obj.boot_mode.default_app_id,
		al_flash_obj_id_to_str(boot_mode_obj.boot_mode.default_app_id));

	return 0;
}

U_BOOT_CMD(
	flash_contents_set_dev, 2, 0, do_flash_contents_set_dev,
	"Set flash contents current device",
	"<boot / spi / nand>");

U_BOOT_CMD(
	flash_contents_toc_print, 1, 0, do_flash_contents_toc_print,
	"Print the flash contents TOC",
	"");

U_BOOT_CMD(
	flash_contents_obj_info_print, 3, 0, do_flash_contents_obj_info_print,
	"Print a specific flash object information",
	"<object ID> <Instance number>");

U_BOOT_CMD(
	flash_contents_obj_read, 4, 0, do_flash_contents_obj_read,
	"Read a specific flash object",
	"<object ID> <Instance number> [Load address]");

U_BOOT_CMD(
	flash_contents_obj_read_mem, 4, 0, do_flash_contents_obj_read_mem,
	"Read a specific flash object from memory",
	"<Load Address> [Read address (default $loadaddr)]");

U_BOOT_CMD(
	flash_contents_obj_validate, 5, 0, do_flash_contents_obj_validate,
	"Validate a specific flash object",
	"<object ID> <Instance number> [Load address] [Size]");

U_BOOT_CMD(
	flash_contents_obj_update, 5, 0, do_flash_contents_obj_update,
	"Update a specific flash object",
	"<object ID> <Instance number> [Load address] [Size]");

U_BOOT_CMD(
	flash_contents_instance_invalidate, 2, 0, do_flash_contents_instance_invalidate,
	"Invalidate a boot instance",
	"<Instance number>");

U_BOOT_CMD(
	flash_contents_instance_revalidate, 2, 0, do_flash_contents_instance_revalidate,
	"Revalidate a boot instance",
	"<Instance number>");

U_BOOT_CMD(
	flash_contents_boot_mode_default_app_set, 2, 0, do_flash_contents_boot_mode_default_app_set,
	"Set default application loaded by the pre-boot code",
	"<object ID>");

U_BOOT_CMD(
	flash_contents_boot_mode_default_app_show, 1, 0,
	do_flash_contents_boot_mode_default_app_show,
	"Show default application loaded by the pre-boot code",
	"");
