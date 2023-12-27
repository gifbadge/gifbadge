#include <esp_adc/adc_oneshot.h>
#include <esp_log.h>
#include "hal/drivers/battery_analog.h"

static const char *TAG = "battery_analog.cpp";


battery_analog::battery_analog(adc_channel_t) {
    adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT_1,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc_handle));
    adc_oneshot_chan_cfg_t config = {
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_9, &config));
    ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &calibration_scheme));
    int reading;
    int voltage;
    adc_oneshot_read(adc_handle, ADC_CHANNEL_9, &reading);
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(calibration_scheme, reading, &voltage));
    present_value = voltage / 1000.00;
    ESP_LOGI(TAG, "Initial Voltage: %f", present_value*2);
    alpha = 0.05;
}

battery_analog::~battery_analog() {

}

BatteryStatus battery_analog::read() {
    int reading;
    int voltage;
    adc_oneshot_read(adc_handle, ADC_CHANNEL_9, &reading);
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(calibration_scheme, reading, &voltage));
    present_value = (voltage / 1000.00) * (alpha) + present_value * (1.0f - alpha);
    return {(present_value * 2), 0, 0};
}

