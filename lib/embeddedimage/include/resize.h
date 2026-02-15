/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include <stdint.h>
#include <utility>
#include "fpm/include/fpm/fixed.hpp"
#include "fpm/include/fpm/math.hpp"

using fixedpt = fpm::fixed<std::int32_t, std::int64_t, 12>;
using weight = fpm::fixed<std::int32_t,  std::int64_t, 12>;

typedef struct
{
  int line = -1;
  uint16_t data[2000] = {};
} rowdata;

class Resize {
  private:
  uint16_t* output;
  uint16_t input_width;
  uint16_t input_height;
  uint16_t resize_width;
  uint16_t resize_height;
  uint16_t frame_width;
  uint16_t frame_height;
  fixedpt ratio;
  int output_y = 0;
    rowdata *rows = nullptr;
  public:
  Resize(uint16_t _input_width, uint16_t _input_height, uint16_t _output_width, uint16_t _output_height, uint16_t* _output, void *buffer);
  int xOffset = 0;
  int yOffset = 0;

  [[nodiscard]] std::pair<int, int> calc_needed_rows(int y) const;
  void line(int input_y, const uint16_t *buf);
};