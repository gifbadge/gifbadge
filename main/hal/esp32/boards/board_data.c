#include <stdint.h>
#include "ota.h"
#include "hal/esp32/boards/boards.h"
#include "sdkconfig.h"

#if CONFIG_SPIRAM_MODE_OCT
const __attribute__((section(".rodata_custom_desc"))) esp_custom_app_desc_t custom_app_desc = {2, {BOARD_2_1_V0_2, BOARD_2_1_V0_4}};
#elif CONFIG_SPIRAM_MODE_QUAD
const __attribute__((section(".rodata_custom_desc"))) esp_custom_app_desc_t custom_app_desc = {2, {BOARD_1_28_V0, BOARD_1_28_V0_1}};
#endif