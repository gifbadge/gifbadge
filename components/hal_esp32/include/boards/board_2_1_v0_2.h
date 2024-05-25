#pragma once
#include <hal/board.h>
#include "drivers/battery_analog.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "drivers/keys_gpio.h"
#include "drivers/display_st7701s.h"
#include "drivers/backlight_ledc.h"
#include "drivers/battery_max17048.h"
#include "drivers/touch_ft5x06.h"
#include "drivers/keys_esp_io_expander.h"
#include "driver/sdmmc_host.h"
#include "drivers/config_nvs.h"
#include "esp_io_expander.h"

class board_2_1_v0_2 : public Board {
 public:
  board_2_1_v0_2();
  ~board_2_1_v0_2() override = default;

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
  bool storageReady() override;
  StorageInfo storageInfo() override;
  int StorageFormat() override { return ESP_OK; };
  const char * name() override;
  bool powerConnected() override;
  void * turboBuffer() override {return nullptr;};
  Config *getConfig() override;
  void debugInfo() override;
  bool usbConnected() override;

 private:
  battery_max17048 * _battery;
  I2C * _i2c;
  keys_esp_io_expander * _keys;
  display_st7701s * _display;
  backlight_ledc * _backlight;
  touch_ft5x06 * _touch;
  sdmmc_card_t *card = nullptr;
  esp_io_expander_handle_t _io_expander = nullptr;
  Config_NVS *_config;
};