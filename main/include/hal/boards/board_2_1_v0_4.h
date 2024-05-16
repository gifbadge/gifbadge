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
#include "hal/drivers/keys_esp_io_expander.h"
#include <driver/sdmmc_host.h>

class board_2_1_v0_4 : public Board {
 public:
  board_2_1_v0_4();
  ~board_2_1_v0_4() override = default;

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
  bool storageReady() override;
  StorageInfo storageInfo() override;
  esp_err_t StorageFormat() override { return ESP_OK; };
  const char * name() override;
  bool powerConnected() override;
  void * turboBuffer() override {return buffer;};



 private:
  battery_max17048 * _battery;
  I2C *_i2c;
  keys_esp_io_expander * _keys;
  display_st7701s * _display;
  backlight_ledc * _backlight;
  touch_ft5x06 * _touch;
  sdmmc_card_t *card = nullptr;
  esp_io_expander_handle_t _io_expander = nullptr;

  typedef struct {
    battery_max17048 * battery;
    esp_io_expander_handle_t io_expander;
  } batteryTimerArgs;

  batteryTimerArgs _batteryTimerArgs;
  static bool checkBatteryInstalled(esp_io_expander_handle_t io_expander);
  static void batteryTimer(void *args);

  void *buffer;

};