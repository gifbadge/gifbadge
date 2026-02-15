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
void hash_path(const char *path, uint8_t *result);

#ifdef __cplusplus
}
#endif