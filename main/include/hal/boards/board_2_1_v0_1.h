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


class board_2_1_v0_1: public Board{
public:
    board_2_1_v0_1();
    ~board_2_1_v0_1() override = default;

    std::shared_ptr<Battery> getBattery() override;
    std::shared_ptr<Touch> getTouch() override;
    std::shared_ptr<I2C> getI2c() override;
    std::shared_ptr<Keys> getKeys() override;
    std::shared_ptr<Display> getDisplay() override;
    std::shared_ptr<Backlight> getBacklight() override;


private:
    std::shared_ptr<battery_max17048> _battery;
    std::shared_ptr<I2C> _i2c;
    std::shared_ptr<keys_gpio> _keys;
    std::shared_ptr<display_st7701s> _display;
    std::shared_ptr<backlight_ledc> _backlight;
};