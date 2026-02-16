/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "bootloader_common.h"
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TEST_MAGIC_VALUE 0x42987561

extern rtc_retain_mem_t* bootloader_common_get_rtc_retain_mem(void);

void app_main(void) {
    rtc_retain_mem_t* mem = bootloader_common_get_rtc_retain_mem();
    uint32_t* _rtc_vars = (uint32_t*) mem->custom;

    if (_rtc_vars[0] != TEST_MAGIC_VALUE) {
        /* On the first boot, set the data inside the array */
        _rtc_vars[0] = TEST_MAGIC_VALUE;
    } else {
        /* Second boot, the data was saved saved, success */
        printf("SUCCESS: data were saved across reboot\n");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }

    esp_restart();
}
