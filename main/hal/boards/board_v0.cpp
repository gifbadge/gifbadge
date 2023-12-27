#include <esp_pm.h>
#include <esp_log.h>


#include "hal/board.h"

#include "hal/boards/board_v0.h"
#include "hal/drivers/backlight_ledc.h"

static const char *TAG = "board_v0";


board_v0::board_v0() {
    _i2c = std::make_shared<I2C>(I2C_NUM_0, 21, 18);
    _battery = std::make_shared<battery_analog>(ADC_CHANNEL_9);
    _keys = std::make_shared<keys_gpio>(GPIO_NUM_43, GPIO_NUM_44, GPIO_NUM_0);
    _display = std::make_shared<display_gc9a01>(35,36,34,37,46);
    _backlight = std::make_shared<backlight_ledc>(GPIO_NUM_45, 0);

    esp_pm_config_t pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 40, .light_sleep_enable = false};
    esp_pm_configure(&pm_config);

    uint8_t data;
    _i2c->read_reg(0x15, 0x0E, &data, 1);
    if(data == 0x2){
        ESP_LOGI(TAG, "MXC4005 accelerometer detected");
    }

}

std::shared_ptr<Battery> board_v0::getBattery() {
    return _battery;
}

std::shared_ptr<Touch> board_v0::getTouch() {
    return nullptr;
}

std::shared_ptr<I2C> board_v0::getI2c(){
    return _i2c;
}

std::shared_ptr<Keys> board_v0::getKeys() {
    return _keys;
}

std::shared_ptr<Display> board_v0::getDisplay() {
    return _display;
}

std::shared_ptr<Backlight> board_v0::getBacklight() {
    return _backlight;
}
