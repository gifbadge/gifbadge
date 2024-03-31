#include <stdint.h>
#include "ota.h"
#include "hal/boards/boards.h"

#ifdef CONFIG_SPIRAM_MODE_OCT
const __attribute__((section(".rodata_custom_desc"))) esp_custom_app_desc_t custom_app_desc = {1, {BOARD_2_1_V0_2}};
#else
const __attribute__((section(".rodata_custom_desc"))) esp_custom_app_desc_t custom_app_desc = {2, {BOARD_1_28_V0, BOARD_1_28_V0_1}};
#endif