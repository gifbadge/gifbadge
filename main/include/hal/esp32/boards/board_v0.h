#pragma once

#include "../../../../../../esp/esp-idf/components/fatfs/src/ff.h"
#include "../../board.h"
#include "hal/esp32/drivers/battery_analog.h"
#include "../../../../../../esp/esp-idf/components/esp_lcd/include/esp_lcd_panel_io.h"
#include "../../../../../../esp/esp-idf/components/esp_lcd/include/esp_lcd_panel_vendor.h"
#include "../../../../../../esp/esp-idf/components/esp_lcd/include/esp_lcd_panel_ops.h"
#include "hal/esp32/drivers/keys_gpio.h"
#include "hal/esp32/drivers/display_gc9a01.h"
#include "hal/esp32/drivers/config_nvs.h"
#include "hal/esp32/i2c.h"

class board_v0 : public Board {
 public:
  board_v0();
  ~board_v0() override = default;

  Battery * getBattery() override;
  Touch * getTouch() override;
  I2C * getI2c();
  Keys * getKeys() override;
  Display * getDisplay() override;
  Backlight * getBacklight() override;

  void powerOff() override;
  void pmLock() override {};
  void pmRelease() override {};
  BOARD_POWER powerState() override;
  bool storageReady() override { return true; };
  StorageInfo storageInfo() override;
  int StorageFormat() override { return ESP_OK; };
  const char *name() override;
  bool powerConnected() override;
  void * turboBuffer() override {return nullptr;};
  Config *getConfig() override;
  void debugInfo() override;
  bool usbConnected() override;

 private:
  battery_analog * _battery;
  I2C * _i2c;
  keys_gpio * _keys;
  display_gc9a01 * _display;
  Backlight * _backlight;
  Config_NVS *_config;
};