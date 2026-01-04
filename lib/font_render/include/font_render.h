/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int render_text_centered(int16_t x_max, int16_t y_max, int16_t margin, const char *text, uint8_t *buffer);
#ifdef __cplusplus
}
#endif