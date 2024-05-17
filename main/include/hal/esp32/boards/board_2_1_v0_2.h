#pragma once
#include "../../board.h"
#include "hal/esp32/drivers/battery_analog.h"
#include "../../../../../../esp/esp-idf/components/esp_lcd/include/esp_lcd_panel_io.h"
#include "../../../../../../esp/esp-idf/components/esp_lcd/include/esp_lcd_panel_vendor.h"
#include "../../../../../../esp/esp-idf/components/esp_lcd/include/esp_lcd_panel_ops.h"
#include "hal/esp32/drivers/keys_gpio.h"
#include "hal/esp32/drivers/display_st7701s.h"
#include "hal/esp32/drivers/backlight_ledc.h"
#include "hal/esp32/drivers/battery_max17048.h"
#include "hal/esp32/drivers/touch_ft5x06.h"
#include "hal/esp32/drivers/keys_esp_io_expander.h"
#include "../../../../../../esp/esp-idf/components/driver/sdmmc/include/driver/sdmmc_host.h"
#include "hal/esp32/drivers/config_nvs.h"

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