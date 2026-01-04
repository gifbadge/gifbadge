/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <cstdint>
#include <utility>

namespace hal::touch {
class Touch {
 public:
  Touch() = default;

  virtual ~Touch() = default;

  virtual std::pair<int16_t, int16_t> read() = 0;
};
}

