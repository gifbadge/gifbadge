#include <esp_pm.h>
#include <esp_log.h>
#include "hal/boards/board_2_1_v0_1.h"
#include "hal/hal_usb.h"

static const char *TAG = "board_2_1_v0_1";

static void IRAM_ATTR sdcard_removed(void *arg){
    esp_restart();
}

board_2_1_v0_1::board_2_1_v0_1() {
    _i2c = std::make_shared<I2C>(I2C_NUM_0, 1, 2);
    _battery = std::make_shared<battery_max17048>(_i2c, GPIO_NUM_21);
    _keys = std::make_shared<keys_gpio>(GPIO_NUM_44, GPIO_NUM_0, GPIO_NUM_41);
    /*G3, G4, G5, R1, R2, R3, R4, R5, B1, B2, B3, B4, B5, G0, G1, G2 */
//    std::array<int, 16> rgb = {11, 12, 13, 14, 15, 16, 17, 18, 3, 4, 5, 6, 7, 8, 9, 10};
    std::array<int, 16> rgb = {11, 12, 13, 3, 4, 5, 6, 7, 14, 15, 16, 17, 18, 8, 9, 10};

    _display = std::make_shared<display_st7701s>(35,3, 4, 48, 47, 33, 34, 39, rgb);
    _backlight = std::make_shared<backlight_ledc>(GPIO_NUM_45, 0);
    _backlight->setLevel(100);


    esp_pm_config_t pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 240, .light_sleep_enable = false};
    esp_pm_configure(&pm_config);

    uint8_t data;
    _i2c->read_reg(0x0F, 0x0F, &data, 1);
    ESP_LOGI(TAG, "KXTJ3-1057 %u", data);

    static sdmmc_card_t *card = nullptr;
    gpio_pullup_en(GPIO_NUM_40);
    if(init_sdmmc_slot(GPIO_NUM_37, GPIO_NUM_38, GPIO_NUM_36, GPIO_NUM_40, &card) == ESP_OK) {
        storage_init_mmc(21, &card);
    }
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_NUM_40, sdcard_removed, nullptr);
    gpio_set_intr_type(GPIO_NUM_40, GPIO_INTR_ANYEDGE);
}


std::shared_ptr<Battery> board_2_1_v0_1::getBattery() {
    return _battery;
}

std::shared_ptr<Touch> board_2_1_v0_1::getTouch() {
    return nullptr;
}

std::shared_ptr<I2C> board_2_1_v0_1::getI2c() {
    return _i2c;
}

std::shared_ptr<Keys> board_2_1_v0_1::getKeys() {
    return _keys;
}

std::shared_ptr<Display> board_2_1_v0_1::getDisplay() {
    return _display;
}

std::shared_ptr<Backlight> board_2_1_v0_1::getBacklight() {
    return _backlight;
}

void board_2_1_v0_1::powerOff() {

}

BOARD_POWER board_2_1_v0_1::powerState() {
    //TODO Detect USB power status, implement critical level
    if(_battery->getSoc() < 10){
        return BOARD_POWER_LOW;
    }
    return BOARD_POWER_NORMAL;
}

bool board_2_1_v0_1::storageReady() {
    if(!gpio_get_level(GPIO_NUM_40)){
        return true;
    }
    return false;
}
