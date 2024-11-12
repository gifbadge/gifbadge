#pragma once

#include "hal/adc_types.h"
#include "esp_adc/adc_oneshot.h"

#include <hal/battery.h>

namespace hal::battery::esp32s3 {
class battery_analog final : public hal::battery::Battery {
 public:
  explicit battery_analog(adc_channel_t);
  ~battery_analog() final;

  void poll();

  double BatteryVoltage() override;

  int BatterySoc() override;;

  void BatteryRemoved() override {};

  void BatteryInserted() override {};

  State BatteryStatus() override { return State::OK ;}

 private:
  adc_oneshot_unit_handle_t adc_handle = nullptr;
  adc_cali_handle_t calibration_scheme = nullptr;
  double smoothed_voltage = 0;
  double alpha = 0.05;
  double scale_factor = 2;
};
}

