#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <esp_adc/adc_oneshot.h>
#include "battery.h"

static const char *TAG = "BATTERY";

void battery_analog_handler(void *params) {
    int reading;
    int voltage;
    auto *args = (AnalogBatteryArgs *) params;
    while (true) {
        adc_oneshot_read(args->adc_handle, ADC_CHANNEL_9, &reading);
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(args->calibration_scheme, reading, &voltage));
        args->m_present_value = (voltage / 1000.00) * (args->m_alpha) + args->m_present_value * (1.0f - args->m_alpha);
        args->battery_config->setVoltage( (args->m_present_value * 2));
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}

void battery_analog_init(const std::shared_ptr<BatteryConfig>& battery_config){
    auto *pBatteryArgs = static_cast<AnalogBatteryArgs *>(malloc(sizeof(AnalogBatteryArgs)));
    pBatteryArgs->battery_config = battery_config;
    adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT_1,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &pBatteryArgs->adc_handle));
    adc_oneshot_chan_cfg_t config = {
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(pBatteryArgs->adc_handle, ADC_CHANNEL_9, &config));
    ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN_DB_11,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &pBatteryArgs->calibration_scheme));
    int reading;
    int voltage;
    adc_oneshot_read(pBatteryArgs->adc_handle, ADC_CHANNEL_9, &reading);
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(pBatteryArgs->calibration_scheme, reading, &voltage));
    pBatteryArgs->m_present_value = voltage / 1000.00;
    pBatteryArgs->m_alpha = 0.05;
    const esp_timer_create_args_t battery_timer_args = {.callback = &battery_analog_handler, .arg = pBatteryArgs, .name = "battery_handler"};
    esp_timer_handle_t battery_handler_handle = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&battery_timer_args, &battery_handler_handle));
    esp_timer_start_periodic(battery_handler_handle, 250*1000);
}
