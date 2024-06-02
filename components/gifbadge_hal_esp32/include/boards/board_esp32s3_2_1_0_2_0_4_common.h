#pragma once
#include <hal/board.h>
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

#include "board_esp32s3_sdmmc.h"

class board_2_1_v0_2v0_4 : public board_esp32s3_sdmmc {
 public:
  board_2_1_v0_2v0_4();
  ~board_2_1_v0_2v0_4() override = default;

  Battery *getBattery() override;
  Touch *getTouch() override;
  Keys *getKeys() override;
  Display *getDisplay() override;
  Backlight *getBacklight() override;

  void powerOff() override;
  BOARD_POWER powerState() override;
  bool storageReady() override;
  bool powerConnected() override;
  void *turboBuffer() override { return buffer; };

 protected:
  I2C *_i2c;
  keys_esp_io_expander *_keys;
  display_st7701s *_display;
  backlight_ledc *_backlight;
  touch_ft5x06 *_touch;
  bool _usbConnected = false;

  void *buffer;
  battery_max17048 *_battery;
  esp_io_expander_handle_t _io_expander = nullptr;
};