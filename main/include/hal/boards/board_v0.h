#pragma once

#include <ff.h>
#include "hal/board.h"
#include "hal/drivers/battery_analog.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "hal/drivers/keys_gpio.h"
#include "hal/drivers/display_gc9a01.h"

class board_v0 : public Board {
 public:
  board_v0();
  ~board_v0() override = default;

  Battery * getBattery() override;
  Touch * getTouch() override;
  I2C * getI2c() override;
  Keys * getKeys() override;
  Display * getDisplay() override;
  Backlight * getBacklight() override;

  void powerOff() override;
  void pmLock() override {};
  void pmRelease() override {};
  BOARD_POWER powerState() override;
  bool storageReady() override { return true; };
  StorageInfo storageInfo() override;
  esp_err_t StorageFormat() override { return ESP_OK; };
  const char *name() override;
  bool powerConnected() override;

 private:
  battery_analog * _battery;
  I2C * _i2c;
  keys_gpio * _keys;
  display_gc9a01 * _display;
  Backlight * _backlight;
};