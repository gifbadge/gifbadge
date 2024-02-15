#include <esp_pm.h>
#include <esp_log.h>
#include <driver/sdmmc_defs.h>
#include <esp_vfs_fat.h>
#include "hal/boards/board_1_28_v0_1.h"
#include "hal/hal_usb.h"
#include "hal/drivers/display_gc9a01.h"

static const char *TAG = "board_1_28_v0_1";

esp_pm_lock_handle_t usb_pm;

static void IRAM_ATTR sdcard_removed(void *arg){
    esp_restart();
}

static void IRAM_ATTR usb_connected(void *arg){
    esp_pm_lock_acquire(usb_pm);
}




board_1_28_v0_1::board_1_28_v0_1() {
    _i2c = std::make_shared<I2C>(I2C_NUM_0, 17, 18);
    _battery = std::make_shared<battery_max17048>(_i2c, GPIO_VBUS_DETECT);
    _keys = std::make_shared<keys_gpio>(GPIO_NUM_0, GPIO_NUM_2, GPIO_NUM_1);
    _display = std::make_shared<display_gc9a01>(35, 36, 34, 37, 38);
    _backlight = std::make_shared<backlight_ledc>(GPIO_NUM_9, 0);
    _backlight->setLevel(100);


//    esp_pm_config_t pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 40, .light_sleep_enable = true};
//    esp_pm_configure(&pm_config);

    gpio_isr_handler_add(GPIO_VBUS_DETECT, usb_connected, nullptr);
    gpio_set_intr_type(GPIO_VBUS_DETECT, GPIO_INTR_HIGH_LEVEL);

    gpio_pullup_en(GPIO_CARD_DETECT);
    if(!gpio_get_level(GPIO_CARD_DETECT)) {
//    if(init_sdmmc_slot(GPIO_NUM_40, GPIO_NUM_39, GPIO_NUM_41, GPIO_NUM_42, GPIO_NUM_33, GPIO_NUM_47, GPIO_CARD_DETECT, &card,
//                       1) == ESP_OK) {
//        storage_init_mmc(GPIO_VBUS_DETECT, &card);
//    }
        mount_sdmmc_slot(GPIO_NUM_40, GPIO_NUM_39, GPIO_NUM_41, GPIO_NUM_42, GPIO_NUM_33, GPIO_NUM_47, GPIO_CARD_DETECT,
                         &card,
                         1);
    }
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_CARD_DETECT, sdcard_removed, nullptr);
    gpio_set_intr_type(GPIO_CARD_DETECT, GPIO_INTR_ANYEDGE);



    //Shutdown pin
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<GPIO_SHUTDOWN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_set_drive_capability(GPIO_SHUTDOWN, GPIO_DRIVE_CAP_MAX);
}


std::shared_ptr<Battery> board_1_28_v0_1::getBattery() {
    return _battery;
}

std::shared_ptr<Touch> board_1_28_v0_1::getTouch() {
    return _touch;
}

std::shared_ptr<I2C> board_1_28_v0_1::getI2c() {
    return _i2c;
}

std::shared_ptr<Keys> board_1_28_v0_1::getKeys() {
    return _keys;
}

std::shared_ptr<Display> board_1_28_v0_1::getDisplay() {
    return _display;
}

std::shared_ptr<Backlight> board_1_28_v0_1::getBacklight() {
    return _backlight;
}

void board_1_28_v0_1::powerOff() {
    ESP_LOGI(TAG, "Poweroff");
    vTaskDelay(100/portTICK_PERIOD_MS);
    gpio_set_level(GPIO_SHUTDOWN, 1);
    gpio_hold_en(GPIO_SHUTDOWN);
}

BOARD_POWER board_1_28_v0_1::powerState() {
    //TODO Detect USB power status, implement critical level
    if(_battery->isCharging()){
        return BOARD_POWER_NORMAL;
    }
    if(_battery->getSoc() < 12){
        if(_battery->getSoc() < 10){
            return BOARD_POWER_CRITICAL;
        }
        return BOARD_POWER_LOW;

    }
    return BOARD_POWER_NORMAL;
}

bool board_1_28_v0_1::storageReady() {
    if(!gpio_get_level(GPIO_CARD_DETECT)){
        return true;
    }
    return false;
}

StorageInfo board_1_28_v0_1::storageInfo() {
    StorageType type = (card->ocr & SD_OCR_SDHC_CAP) ? StorageType_SDHC : StorageType_SD;
    double speed = card->real_freq_khz/1000;
    uint64_t total_bytes;
    uint64_t free_bytes;
    esp_vfs_fat_info("/data", &total_bytes, &free_bytes);
    return {card->cid.name, type, speed, total_bytes, free_bytes};
}
