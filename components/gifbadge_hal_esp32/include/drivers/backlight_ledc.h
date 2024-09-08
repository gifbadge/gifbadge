#pragma once

#include <hal/backlight.h>

class backlight_ledc : public Backlight {
 public:
  explicit backlight_ledc(gpio_num_t gpio, bool invert, int level = 100);
  ~backlight_ledc() override = default;
  void state(bool) override;
  void setLevel(int) override;
  int getLevel() override;
 private:
  int lastLevel;
};