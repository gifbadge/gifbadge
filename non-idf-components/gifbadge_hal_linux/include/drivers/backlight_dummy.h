#pragma once

#include "hal/backlight.h"

namespace hal::backlight::linux {
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

