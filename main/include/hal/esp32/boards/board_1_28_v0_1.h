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
#include "hal/esp32/drivers/display_gc9a01.h"
#include "../../../../../../esp/esp-idf/components/driver/sdmmc/include/driver/sdmmc_host.h"
#include "../../../../../../esp/esp-idf/components/esp_pm/include/esp_pm.h"
#include "hal/esp32/drivers/config_nvs.h"

#define GPIO_CARD_DETECT GPIO_NUM_21
#define GPIO_VBUS_DETECT GPIO_NUM_6
#define GPIO_SHUTDOWN GPIO_NUM_7

class board_1_28_v0_1 : public Board {
 public:
  board_1_28_v0_1();
  ~board_1_28_v0_1() override = default;

  Battery * getBattery() override;
  Touch * getTouch() override;
  I2C * getI2c();
  Keys * getKeys() override;
  Display * getDisplay() override;
  Backlight * getBacklight() override;

  void powerOff() override;
  void pmLock() override;
  void pmRelease() override;

  BOARD_POWER powerState() override;
  bool storageReady() override;
  StorageInfo storageInfo() override;
  int StorageFormat() override;
  const char * name() override;
  bool powerConnected() override;
  void * turboBuffer() override {return buffer;};
  Config *getConfig() override;


 private:
  battery_max17048 *_battery;
  I2C *_i2c;
  keys_gpio * _keys;
  display_gc9a01 * _display;
  backlight_ledc * _backlight;
  touch_ft5x06 * _touch;
  sdmmc_card_t *card = nullptr;
  esp_pm_lock_handle_t pmLockHandle = nullptr;
  void *buffer;
  Config_NVS *_config;
};