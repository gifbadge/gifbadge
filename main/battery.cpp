#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <esp_adc/adc_oneshot.h>
#include "battery.h"

static const char *TAG = "BATTERY";

void battery_handler(void *params){
    auto *args = (BatteryArgs *) params;
    BatteryStatus status = args->battery->read();
    args->battery_config->setVoltage(status.voltage);
}

void battery_init(const std::shared_ptr<Battery>& battery, const std::shared_ptr<BatteryConfig>& battery_config){
    auto *pBatteryArgs = static_cast<BatteryArgs *>(malloc(sizeof(BatteryArgs)));
    pBatteryArgs->battery = battery;
    pBatteryArgs->battery_config = battery_config;

    const esp_timer_create_args_t battery_timer_args = {.callback = &battery_handler, .arg = pBatteryArgs, .name = "battery_handler"};
    esp_timer_handle_t battery_handler_handle = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&battery_timer_args, &battery_handler_handle));
    esp_timer_start_periodic(battery_handler_handle, battery->pollInterval()*1000);
}