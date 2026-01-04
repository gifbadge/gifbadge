/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

namespace hal::backlight{
class Backlight {
 public:
  Backlight() = default;
  virtual ~Backlight() = default;
  virtual void state(bool) = 0;
  /*!
   * set backlight level in %
   */
  virtual void setLevel(int) = 0;
  virtual int getLevel() = 0;
};
}

