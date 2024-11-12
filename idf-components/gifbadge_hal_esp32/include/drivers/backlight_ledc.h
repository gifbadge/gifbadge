#pragma once

#include <hal/backlight.h>

namespace hal::backlight::esp32s3 {
class backlight_ledc : public hal::backlight::Backlight {
 public:
  explicit backlight_ledc(gpio_num_t gpio, bool invert, int level = 100);
  ~backlight_ledc() override = default;
  void state(bool) override;
  void setLevel(int) override;
  int getLevel() override;
 private:
  int lastLevel;
};
}

