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

class board_2_1_v0_2 : public Board {
 public:
  board_2_1_v0_2();
  ~board_2_1_v0_2() override = default;

  std::shared_ptr<Battery> getBattery() override;
  std::shared_ptr<Touch> getTouch() override;
  std::shared_ptr<I2C> getI2c() override;
  std::shared_ptr<Keys> getKeys() override;
  std::shared_ptr<Display> getDisplay() override;
  std::shared_ptr<Backlight> getBacklight() override;

  void powerOff() override;
  void pmLock() override {};
  void pmRelease() override {};
  BOARD_POWER powerState() override;
  bool storageReady() override;
  StorageInfo storageInfo() override;
  esp_err_t StorageFormat() override { return ESP_OK; };
  std::string name() override;
  bool powerConnected() override;

 private:
  std::shared_ptr<battery_max17048> _battery;
  std::shared_ptr<I2C> _i2c;
  std::shared_ptr<keys_esp_io_expander> _keys;
  std::shared_ptr<display_st7701s> _display;
  std::shared_ptr<backlight_ledc> _backlight;
  std::shared_ptr<touch_ft5x06> _touch;
  sdmmc_card_t *card = nullptr;
  esp_io_expander_handle_t _io_expander = nullptr;
};