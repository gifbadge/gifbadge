#pragma once
#include "hal/board.h"
#include "hal/drivers/battery_analog.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "hal/drivers/keys_gpio.h"
#include "hal/drivers/display_st7701s.h"
#include "hal/drivers/backlight_ledc.h"
#include "hal/drivers/battery_max17048.h"
#include "hal/drivers/touch_ft5x06.h"
#include "hal/drivers/display_gc9a01.h"
#include <driver/sdmmc_host.h>
#include <esp_pm.h>

#define GPIO_CARD_DETECT GPIO_NUM_21
#define GPIO_VBUS_DETECT GPIO_NUM_6
#define GPIO_SHUTDOWN GPIO_NUM_7

class board_1_28_v0_1: public Board{
public:
    board_1_28_v0_1();
    ~board_1_28_v0_1() override = default;

    std::shared_ptr<Battery> getBattery() override;
    std::shared_ptr<Touch> getTouch() override;
    std::shared_ptr<I2C> getI2c() override;
    std::shared_ptr<Keys> getKeys() override;
    std::shared_ptr<Display> getDisplay() override;
    std::shared_ptr<Backlight> getBacklight() override;

    void powerOff() override;
    void pmLock() override;
    void pmRelease() override;

    BOARD_POWER powerState() override;
    bool storageReady() override;
    StorageInfo storageInfo() override;
    esp_err_t StorageFormat() override;




private:
    std::shared_ptr<battery_max17048> _battery;
    std::shared_ptr<I2C> _i2c;
    std::shared_ptr<keys_gpio> _keys;
    std::shared_ptr<display_gc9a01> _display;
    std::shared_ptr<backlight_ledc> _backlight;
    std::shared_ptr<touch_ft5x06> _touch;
    sdmmc_card_t *card = nullptr;
    SemaphoreHandle_t pmLockCount;
    esp_pm_lock_handle_t pmLockHandle;
};