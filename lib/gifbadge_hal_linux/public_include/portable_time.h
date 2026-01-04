/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
int64_t get_millis();
#ifdef __cplusplus
}
#endif
#define millis() get_millis()
