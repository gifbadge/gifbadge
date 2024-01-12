#include "hal/drivers/battery_max17048.h"

#include <utility>
#include <esp_log.h>

static const char *TAG = "battery_max17048";


battery_max17048::battery_max17048(std::shared_ptr<I2C> i2c, gpio_num_t vbus_pin): _i2c(std::move(i2c)), _vbus_pin(vbus_pin) {
    uint8_t data[2];
    _i2c->read_reg(0x36, 0x08, data, 2);
    ESP_LOGI(TAG, "MAX17048 Version %u %u", data[0], data[1]);
    const esp_timer_create_args_t battery_timer_args = {
            .callback = [](void *params){auto bat = (battery_max17048 *) params; bat->read();},
            .arg = this,
            .name = "battery_max17048"
    };
    esp_timer_handle_t battery_handler_handle = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&battery_timer_args, &battery_handler_handle));
    esp_timer_start_periodic(battery_handler_handle, pollInterval()*1000);
}

battery_max17048::~battery_max17048() {

}

BatteryStatus battery_max17048::read() {
    uint8_t d[2];
    _i2c->read_reg(0x36, 0x02, d, 2);
    _voltage = ((static_cast<uint16_t>(d[0] << 8)|d[1])*78.125)/1000000;
//    ESP_LOGI(TAG, "MAX17048 Voltage %f ", voltage);
    _i2c->read_reg(0x36, 0x04, d, 2);
    _soc = static_cast<int>((static_cast<uint16_t>(d[0] << 8)|d[1])/256.00);
//    ESP_LOGI(TAG, "MAX17048 SOC %i ", _soc);
    _i2c->read_reg(0x36, 0x16, d, 2);
    _rate = (static_cast<int16_t>(d[0] << 8)|d[1])*0.208;
//    ESP_LOGI(TAG, "MAX17048 CRATE %f ", change);
    return {_voltage, _soc, _rate};
}

double battery_max17048::getVoltage() {
    return _voltage;
}

int battery_max17048::getSoc() {
    return _soc;
}

double battery_max17048::getRate() {
    return _rate;
}

bool battery_max17048::isCharging() {
    return gpio_get_level(_vbus_pin);
}
