#pragma once

#include "hal/backlight.h"

class backlight_dummy : public Backlight {
 public:
  explicit backlight_dummy();
  ~backlight_dummy() override = default;
  void state(bool) override;
  void setLevel(int) override;
  int getLevel() override;
 private:
  int lastLevel = 100;
};