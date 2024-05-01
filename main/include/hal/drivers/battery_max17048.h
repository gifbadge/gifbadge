#pragma once

#include <hal/adc_types.h>
#include <esp_adc/adc_oneshot.h>
#include <memory>

#include "hal/battery.h"
#include "hal/i2c.h"

class battery_max17048 final : public Battery {
 public:
  explicit battery_max17048(std::shared_ptr<I2C>, gpio_num_t vbus_pin);
  ~battery_max17048() final = default;

  void poll();

  int pollInterval() override { return 1000; };

  double getVoltage() override;

  int getSoc() override;

  double getRate() override;

  void removed() override;

  void inserted() override;

  State status() override;

 private:
  std::shared_ptr<I2C> _i2c;
  double _voltage = 0;
  int _soc = 0;
  double _rate = 0;
  gpio_num_t _vbus_pin;
  bool present = false;
};