#pragma once

#include <memory>

#include <hal/battery.h>
#include "i2c.h"

namespace hal::battery::esp32s3 {
class battery_max17048 final : public hal::battery::Battery {
 public:
  explicit battery_max17048(I2C *, gpio_num_t vbus_pin);
  ~battery_max17048() final = default;

  void poll();

  double BatteryVoltage() override;

  int BatterySoc() override;

  void BatteryRemoved() override;

  void BatteryInserted() override;

  State BatteryStatus() override;

 private:
  I2C *_i2c;
  double _voltage = 0;
  int _soc = 0;
  double _rate = 0;
  gpio_num_t _vbus_pin;
  bool present = false;
};
}

