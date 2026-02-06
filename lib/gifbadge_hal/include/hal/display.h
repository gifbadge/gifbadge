/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include <stdint.h>
#include <utility>
typedef bool (*flushCallback_t)();

namespace hal::display {
class Display {
 public:
  Display() = default;
  virtual ~Display() = default;

  virtual bool onColorTransDone(flushCallback_t) = 0;
  virtual void write(int x_start, int y_start, int x_end, int y_end, void *color_data) = 0;
  virtual void clear() = 0;

  uint8_t *buffer = nullptr;
  std::pair<int16_t, int16_t> size;

};
}

