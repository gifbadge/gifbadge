#pragma once

#include <hal/adc_types.h>
#include <esp_adc/adc_oneshot.h>

#include "hal/battery.h"

class battery_analog final : public Battery {
 public:
  explicit battery_analog(adc_channel_t);
  ~battery_analog() final;

  void poll();

  int pollInterval() override { return 250; };

  double getVoltage() override;

  int getSoc() override;

  double getRate() override { return 0; };

  void removed() override {};

  void inserted() override {};

  Battery::State status() override { return Battery::State::OK ;}

 private:
  adc_oneshot_unit_handle_t adc_handle = nullptr;
  adc_cali_handle_t calibration_scheme = nullptr;
  double smoothed_voltage = 0;
  double alpha = 0.05;
  double scale_factor = 2;
};