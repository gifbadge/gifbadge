#include <esp_pm.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <wear_levelling.h>


#include "hal/board.h"

#include "hal/boards/board_v0.h"
#include "hal/drivers/backlight_ledc.h"
#include "hal/hal_usb.h"

static const char *TAG = "board_v0";


board_v0::board_v0() {
    _i2c = std::make_shared<I2C>(I2C_NUM_0, 21, 18);
    _battery = std::make_shared<battery_analog>(ADC_CHANNEL_9);
    _keys = std::make_shared<keys_gpio>(GPIO_NUM_43, GPIO_NUM_44, GPIO_NUM_0);
    _display = std::make_shared<display_gc9a01>(35,36,34,37,46);
    _backlight = std::make_shared<backlight_ledc>(GPIO_NUM_45, 0);

    static wl_handle_t wl_handle = WL_INVALID_HANDLE;
    ESP_ERROR_CHECK(storage_init_ext_flash(39, 41, 40, 42, &wl_handle));


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

void board_v0::powerOff() {
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    rtc_gpio_pullup_en(static_cast<gpio_num_t>(0));
    rtc_gpio_pulldown_dis(static_cast<gpio_num_t>(0));
    esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(0), 0);
    esp_deep_sleep_start();
}

BOARD_POWER board_v0::powerState() {
    return BOARD_POWER_NORMAL;
}

StorageInfo board_v0::storageInfo() {
    return StorageInfo();
}
