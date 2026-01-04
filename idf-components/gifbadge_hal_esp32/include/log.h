/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include "esp_log.h"
#define LOGE( tag, format, ... ) ESP_LOGE( tag, format, ##__VA_ARGS__ )
#define LOGW( tag, format, ... ) ESP_LOGW( tag, format, ##__VA_ARGS__ )
#define LOGI( tag, format, ... ) ESP_LOGI( tag, format, ##__VA_ARGS__ )
#define LOGD( tag, format, ... ) ESP_LOGD( tag, format, ##__VA_ARGS__ )
#define LOGV( tag, format, ... ) ESP_LOGV( tag, format, ##__VA_ARGS__ )

