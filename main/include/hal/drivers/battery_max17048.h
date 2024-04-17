#pragma once

#include <hal/adc_types.h>
#include <esp_adc/adc_oneshot.h>
#include <memory>

#include "hal/battery.h"
#include "hal/i2c.h"

class battery_max17048 final : public Battery {
 public:
  explicit battery_max17048(std::shared_ptr<I2C>, gpio_num_t vbus_pin);
  ~battery_max17048() final;

  BatteryStatus read() final;

  int pollInterval() override { return 1000; };

  double getVoltage() override;

  int getSoc() override;

  double getRate() override;

  bool isCharging() override;

 private:
  std::shared_ptr<I2C> _i2c;
  double _voltage;
  int _soc;
  double _rate;
  gpio_num_t _vbus_pin;
};