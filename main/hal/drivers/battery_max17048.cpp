#include "hal/drivers/battery_max17048.h"

#include <utility>
#include <esp_log.h>

static const char *TAG = "battery_max17048";


battery_max17048::battery_max17048(std::shared_ptr<I2C> i2c): _i2c(std::move(i2c)) {
    uint8_t data[2];
    _i2c->read_reg(0x36, 0x08, data, 2);
    ESP_LOGI(TAG, "MAX17048 Version %u %u", data[0], data[1]);

}

battery_max17048::~battery_max17048() {

}

BatteryStatus battery_max17048::read() {
    uint8_t d[2];
    _i2c->read_reg(0x36, 0x02, d, 2);
    _voltage = (__bswap16(*reinterpret_cast<uint16_t *>(d))*78.125)/1000000;
//    ESP_LOGI(TAG, "MAX17048 Voltage %f ", voltage);
    _i2c->read_reg(0x36, 0x04, d, 2);
    _soc = static_cast<int>(__bswap16(*reinterpret_cast<uint16_t *>(d))/256.00);
//    ESP_LOGI(TAG, "MAX17048 SOC %f ", soc);
    _i2c->read_reg(0x36, 0x16, d, 2);
    _rate = __bswap16(*reinterpret_cast<int16_t *>(d))*0.208;
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
