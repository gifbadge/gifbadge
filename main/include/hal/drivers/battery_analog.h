#pragma once

#include <hal/adc_types.h>
#include <esp_adc/adc_oneshot.h>

#include "hal/battery.h"

class battery_analog final: public Battery{
public:
    explicit battery_analog(adc_channel_t);
    ~battery_analog() final;

    BatteryStatus read() final;

    int pollInterval() override { return 250;};

private:
    adc_oneshot_unit_handle_t adc_handle;
    adc_cali_handle_t calibration_scheme;
    double present_value = 0;
    double alpha = 0.05;
};