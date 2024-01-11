#pragma once
#include "hal/board.h"
#include "hal/drivers/battery_analog.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "hal/drivers/keys_gpio.h"
#include "hal/drivers/display_gc9a01.h"


class board_v0: public Board{
public:
    board_v0();
    ~board_v0() override = default;

    std::shared_ptr<Battery> getBattery() override;
    std::shared_ptr<Touch> getTouch() override;
    std::shared_ptr<I2C> getI2c() override;
    std::shared_ptr<Keys> getKeys() override;
    std::shared_ptr<Display> getDisplay() override;
    std::shared_ptr<Backlight> getBacklight() override;

    void powerOff() override;
    BOARD_POWER powerState() override;
    bool storageReady() override {return true;};


private:
    std::shared_ptr<battery_analog> _battery;
    std::shared_ptr<I2C> _i2c;
    std::shared_ptr<keys_gpio> _keys;
    std::shared_ptr<display_gc9a01> _display;
    std::shared_ptr<Backlight> _backlight;
};