#pragma once

#include "ff.h"
#include <hal/board.h>
#include "drivers/battery_analog.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "drivers/keys_gpio.h"
#include "drivers/display_gc9a01.h"
#include "drivers/config_nvs.h"
#include "i2c.h"
#include "boards_esp32s3.h"

namespace Boards {
class v0 : public Boards::esp32s3 {
 public:
  v0();
  ~v0() override = default;

  Battery * getBattery() override;
  Touch * getTouch() override;
  Keys * getKeys() override;
  Display * getDisplay() override;
  Backlight * getBacklight() override;

  void powerOff() override;
  BOARD_POWER powerState() override;
  bool storageReady() override { return true; };
  StorageInfo storageInfo() override;
  int StorageFormat() override { return ESP_OK; };
  const char *name() override;
  CHARGE_POWER powerConnected() override;
  void * turboBuffer() override {return nullptr;};
  bool usbConnected() override;

 private:
  battery_analog * _battery;
  I2C * _i2c;
  keys_gpio * _keys;
  display_gc9a01 * _display;
  Backlight * _backlight;
};
}