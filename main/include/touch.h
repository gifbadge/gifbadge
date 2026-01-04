/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

struct touch_event {
  int64_t timestamp;
  uint8_t point;
  uint16_t x;
  uint16_t y;
};