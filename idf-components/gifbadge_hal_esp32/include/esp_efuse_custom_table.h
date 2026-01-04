/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_efuse.h"

// md5_digest_table 1cc13c25161e1fe36796db43a91ab759
// This file was generated from the file esp_efuse_custom_table.csv. DO NOT CHANGE THIS FILE MANUALLY.
// If you want to change some fields, you need to change esp_efuse_custom_table.csv file
// then run `efuse_common_table` or `efuse_custom_table` command it will generate this file.
// To show efuse_table run the command 'show_efuse_table'.


extern const esp_efuse_desc_t* ESP_EFUSE_USER_DATA_BOARD[];
extern const esp_efuse_desc_t* ESP_EFUSE_KEY0_SERIAL[];
extern const esp_efuse_desc_t* ESP_EFUSE_KEY1_SERIAL[];

#ifdef __cplusplus
}
#endif
