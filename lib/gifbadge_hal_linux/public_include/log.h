/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include "console.h"
#define LOGI(tag, format, ...) console_print(tag, format, ##__VA_ARGS__)

