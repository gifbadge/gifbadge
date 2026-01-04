/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include "hal/board.h"
#include "drivers/battery_analog.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "drivers/keys_gpio.h"
#include "drivers/display_st7701s.h"
#include "drivers/backlight_ledc.h"
#include "drivers/battery_max17048.h"
#include "drivers/touch_ft5x06.h"
#include "drivers/display_gc9a01.h"
#include "driver/sdmmc_host.h"
#include "esp_pm.h"
#include "drivers/config_nvs.h"
#include "soc/gpio_num.h"
#include "boards/esp32s3_sdmmc.h"
#include "drivers/vbus_gpio.h"

namespace Boards::esp32::s3::mini {
class v0_1 : public esp32s3_sdmmc {
 public:
  v0_1();
  ~v0_1() override = default;

  hal::battery::Battery * GetBattery() override;
  hal::touch::Touch * GetTouch() override;
  hal::keys::Keys * GetKeys() override;
  hal::display::Display * GetDisplay() override;
  hal::backlight::Backlight * GetBacklight() override;

  void PowerOff() override;

  BoardPower PowerState() override;
  bool StorageReady() override;
  const char * Name() override;
  void * TurboBuffer() override {return buffer;}
  void LateInit() override;
  hal::vbus::Vbus *GetVbus() override;;

 private:
  hal::battery::esp32s3::battery_max17048 *_battery = nullptr;
  i2c_master_bus_handle_t bus_handle = nullptr;
  hal::keys::esp32s3::keys_gpio * _keys = nullptr;
  hal::display::esp32s3::display_gc9a01 * _display = nullptr;
  hal::backlight::esp32s3::backlight_ledc * _backlight = nullptr;
  hal::touch::esp32s3::touch_ft5x06 * _touch = nullptr;
  void *buffer = nullptr;
  hal::vbus::esp32s3::VbusGpio *_vbus = nullptr;
};
}
