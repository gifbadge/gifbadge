#include <esp_pm.h>
#include <esp_log.h>
#include <driver/sdmmc_defs.h>
#include <esp_vfs_fat.h>
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

    gpio_config_t display_reset_conf = {
        .pin_bit_mask = 1ULL << 39,
        .mode = GPIO_MODE_OUTPUT,
    };
  ESP_ERROR_CHECK(gpio_config(&display_reset_conf));

  gpio_set_level(GPIO_NUM_39, 0);
  vTaskDelay(pdMS_TO_TICKS(10));
  gpio_set_level(GPIO_NUM_39, 1);

      spi_line_config_t line_config = {
            .cs_io_type = IO_TYPE_GPIO,             // Set to `IO_TYPE_GPIO` if using GPIO, same to below
            .cs_gpio_num = 35,
            .scl_io_type = IO_TYPE_GPIO,
            .scl_gpio_num = 3,
            .sda_io_type = IO_TYPE_GPIO,
            .sda_gpio_num = 4,
            .io_expander = nullptr,                        // Set to NULL if not using IO expander
    };

    _display = std::make_shared<display_st7701s>(line_config, 48, 47, 33, 34, 39, rgb);
    _backlight = std::make_shared<backlight_ledc>(GPIO_NUM_45, 0);
    _backlight->setLevel(100);
    _touch = std::make_shared<touch_ft5x06>(_i2c);


    esp_pm_config_t pm_config = {.max_freq_mhz = 240, .min_freq_mhz = 240, .light_sleep_enable = false};
    esp_pm_configure(&pm_config);

    uint8_t data = 0;
    if(_i2c->write_reg(0x0D, 0x7F, &data, 1) == ESP_OK) {
        _i2c->read_reg(0x0D, 0x0C, &data, 1);
        ESP_LOGI(TAG, "KXTJ3-1057 %u", data);
    }
    else if(_i2c->write_reg(0x0F, 0x7F, &data, 1) == ESP_OK) {
        _i2c->read_reg(0x0F, 0x0C, &data, 1);
        ESP_LOGI(TAG, "KXTJ3-1057 %u", data);
    }
    else if(_i2c->write_reg(0x0C, 0x7F, &data, 1) == ESP_OK) {
        _i2c->read_reg(0x0C, 0x0C, &data, 1);
        ESP_LOGI(TAG, "KXTJ3-1057 %u", data);
    }
    else if(_i2c->write_reg(0x0E, 0x7F, &data, 1) == ESP_OK) {
        _i2c->read_reg(0x0E, 0x0C, &data, 1);
        ESP_LOGI(TAG, "KXTJ3-1057 %u", data);
    }
    else {
        ESP_LOGI(TAG, "KXTJ3-1057 not responding");
    }




    gpio_pullup_en(GPIO_NUM_40);
    if(init_sdmmc_slot(GPIO_NUM_37, GPIO_NUM_38, GPIO_NUM_36, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_40, &card,
                     1) == ESP_OK) {
      usb_init_mmc(21, &card);
    }
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_NUM_40, sdcard_removed, nullptr);
    gpio_set_intr_type(GPIO_NUM_40, GPIO_INTR_ANYEDGE);

    //Shutdown pin
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<42);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_set_drive_capability(GPIO_NUM_42, GPIO_DRIVE_CAP_MAX);
}


std::shared_ptr<Battery> board_2_1_v0_1::getBattery() {
    return _battery;
}

std::shared_ptr<Touch> board_2_1_v0_1::getTouch() {
    return _touch;
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
    ESP_LOGI(TAG, "Poweroff");
    vTaskDelay(100/portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_42, 1);
    gpio_hold_en(GPIO_NUM_42);
}

BOARD_POWER board_2_1_v0_1::powerState() {
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

bool board_2_1_v0_1::storageReady() {
    if(!gpio_get_level(GPIO_NUM_40)){
        return true;
    }
    return false;
}

StorageInfo board_2_1_v0_1::storageInfo() {
    StorageType type = (card->ocr & SD_OCR_SDHC_CAP) ? StorageType_SDHC : StorageType_SD;
    double speed = card->real_freq_khz/1000;
    uint64_t total_bytes;
    uint64_t free_bytes;
    esp_vfs_fat_info("/data", &total_bytes, &free_bytes);
    return {card->cid.name, type, speed, total_bytes, free_bytes};
}
