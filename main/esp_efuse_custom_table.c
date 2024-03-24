/*
 * SPDX-FileCopyrightText: 2017-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"
#include "esp_efuse.h"
#include <assert.h>
#include "esp_efuse_custom_table.h"

// md5_digest_table 4a1734a4aed931971168b0384ae86118
// This file was generated from the file esp_efuse_custom_table.csv. DO NOT CHANGE THIS FILE MANUALLY.
// If you want to change some fields, you need to change esp_efuse_custom_table.csv file
// then run `efuse_common_table` or `efuse_custom_table` command it will generate this file.
// To show efuse_table run the command 'show_efuse_table'.

static const esp_efuse_desc_t USER_DATA_BOARD[] = {
    {EFUSE_BLK3, 0, 8}, 	 // Board Type,
};





const esp_efuse_desc_t* ESP_EFUSE_USER_DATA_BOARD[] = {
    &USER_DATA_BOARD[0],    		// Board Type
    NULL
};
