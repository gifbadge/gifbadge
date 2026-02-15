/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "hash_path.h"
#include "md5.h"

void hash_path(const char *path, uint8_t *result) {
  md5String(path, result);
}