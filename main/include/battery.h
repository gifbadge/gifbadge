#pragma once
#include "config.h"

struct AnalogBatteryArgs {
    std::shared_ptr<BatteryConfig> battery_config;
    adc_oneshot_unit_handle_t adc_handle;
    adc_cali_handle_t calibration_scheme;
    double m_present_value = 0;
    double m_alpha = 0.05;
};

void battery_analog_init(const std::shared_ptr<BatteryConfig>& battery_config);