#pragma once

#include <memory>
#include <driver/i2c_types.h>
#include <hal/battery.h>
#include <soc/gpio_num.h>


namespace hal::battery::esp32s3 {
class battery_max17048 final : public hal::battery::Battery {
 public:
  explicit battery_max17048(i2c_master_bus_handle_t, gpio_num_t vbus_pin);
  ~battery_max17048() final = default;

  void poll();

  double BatteryVoltage() override;

  int BatterySoc() override;

  void BatteryRemoved() override;

  void BatteryInserted() override;

  State BatteryStatus() override;

 private:
  i2c_master_bus_handle_t i2c_master;
  i2c_master_dev_handle_t i2c_handle = nullptr;
  double _voltage = 0;
  int _soc = 0;
  double _rate = 0;
  gpio_num_t _vbus_pin;
  bool present = false;
};
}

