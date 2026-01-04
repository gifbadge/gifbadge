/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include "hal/backlight.h"

namespace hal::backlight::oslinux {
class backlight_dummy : public hal::backlight::Backlight {
 public:
  backlight_dummy();
  ~backlight_dummy() override = default;
  void state(bool) override;
  void setLevel(int) override;
  int getLevel() override;
 private:
  int lastLevel = 100;
};
}

