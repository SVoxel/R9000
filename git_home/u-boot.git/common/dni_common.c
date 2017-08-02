#include <common.h>
#include <config.h>
#include <dni_common.h>

#if defined(NETGEAR_BOARD_ID_SUPPORT)
#if defined(CONFIG_USE_COMMON_BOARD_ID_IMPLEMENTATION)
/*
 * item_name_want could be "device" to get Model Id, "version" to get Version
 * or "hd_id" to get Hardware ID.
 */
void board_get_image_info(ulong fw_image_addr, char *item_name_want, char *buf)
{
	char image_header[HEADER_LEN+1];
	char item_name[HEADER_LEN+1];
	char *item_value;
	char *parsing_string;

	memset(image_header, 0, HEADER_LEN);
	memcpy(image_header, fw_image_addr, HEADER_LEN);
	image_header[HEADER_LEN]='\0';

	parsing_string = strtok(image_header, "\n");
	while (parsing_string != NULL) {
		char *colon_p;
		memset(item_name, 0, sizeof(item_name));
		colon_p = strchr(parsing_string, ':');
		if (colon_p == NULL) {
			break;
		}
		strncpy(item_name, parsing_string, (int)(colon_p - parsing_string));

		if (strcmp(item_name, item_name_want) == 0) {
			item_value = strstr(parsing_string, ":") + 1;

			memcpy(buf, item_value, strlen(item_value));
		}

		parsing_string = strtok(NULL, "\n");
	}
}

int board_match_image_hw_id (ulong fw_image_addr)
{
	char board_hw_id[BOARD_HW_ID_LENGTH + 1];
	char image_hw_id[BOARD_HW_ID_LENGTH + 1];

	/*get hardward id from board */
	memset(board_hw_id, 0, sizeof(board_hw_id));
	get_board_data(BOARD_HW_ID_OFFSET, BOARD_HW_ID_LENGTH, (u8 *)board_hw_id);
	printf("HW ID on board: %s\n", board_hw_id);

	/*get hardward id from image */
	memset(image_hw_id, 0, sizeof(image_hw_id));
	board_get_image_info(fw_image_addr, "hd_id", image_hw_id);
	printf("HW ID on image: %s\n", image_hw_id);

	if (memcmp(board_hw_id, image_hw_id, BOARD_HW_ID_LENGTH) != 0) {
        printf("Firmware Image HW ID do not match Board HW ID\n");
        return 0;
	}
	printf("Firmware Image HW ID matched Board HW ID\n\n");
	return 1;
}

int board_match_image_model_id (ulong fw_image_addr)
{
	char board_model_id[BOARD_MODEL_ID_LENGTH + 1];
	char image_model_id[BOARD_MODEL_ID_LENGTH + 1];

	/*get hardward id from board */
	memset(board_model_id, 0, sizeof(board_model_id));
	get_board_data(BOARD_MODEL_ID_OFFSET, BOARD_MODEL_ID_LENGTH, (u8 *)board_model_id);
	printf("MODEL ID on board: %s\n", board_model_id);

	/*get hardward id from image */
	memset(image_model_id, 0, sizeof(image_model_id));
	board_get_image_info(fw_image_addr, "device", image_model_id);
	printf("MODEL ID on image: %s\n", image_model_id);

	if (memcmp(board_model_id, image_model_id, BOARD_MODEL_ID_LENGTH) != 0) {
        printf("Firmware Image MODEL ID do not match Board model ID\n");
        return 0;
	}
	printf("Firmware Image MODEL ID matched Board model ID\n\n");
	return 1;
}

void board_update_image_model_id (ulong fw_image_addr)
{
	char board_model_id[BOARD_MODEL_ID_LENGTH + 1];
	char image_model_id[BOARD_MODEL_ID_LENGTH + 1];

	/*get model id from board */
	memset(board_model_id, 0, sizeof(board_model_id));
	get_board_data(BOARD_MODEL_ID_OFFSET, BOARD_MODEL_ID_LENGTH, board_model_id);
	printf("Original board MODEL ID: %s\n", board_model_id);

	/*get model id from image */
	memset(image_model_id, 0, sizeof(image_model_id));
	board_get_image_info(fw_image_addr, "device", image_model_id);
	printf("New MODEL ID from image: %s\n", image_model_id);

	printf("Updating MODEL ID\n");
	set_board_data(BOARD_MODEL_ID_OFFSET, BOARD_MODEL_ID_LENGTH, image_model_id);

	printf("done\n\n");
}
#endif

#if defined(OPEN_SOURCE_ROUTER_SUPPORT) && defined(OPEN_SOURCE_ROUTER_ID)
int  image_match_open_source_fw_id (ulong fw_image_addr)
{
	char image_model_id[BOARD_MODEL_ID_LENGTH + 1];

	/*get hardward id from image */
	memset(image_model_id, 0, sizeof(image_model_id));
	board_get_image_info(fw_image_addr, "device", (char*)image_model_id);
	printf("MODEL ID on image: %s\n", image_model_id);

	if (strcmp(image_model_id, OPEN_SOURCE_ROUTER_ID) != 0) {
		printf("Firmware Image MODEL ID do not match open source firmware ID\n");
		return 0;
	}
	printf("Firmware Image MODEL ID matched open source firmware ID\n\n");
	return 1;
}
#endif
#endif	/* BOARD_ID */
