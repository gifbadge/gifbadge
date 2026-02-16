/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include <drivers/backlight_dummy.h>
#include <drivers/display_dummy.h>

#include "hal/board.h"
#include "esp_lcd_panel_io.h"
#include "drivers/keys_gpio.h"
#include "drivers/battery_max17048.h"
#include "drivers/touch_ft5x06.h"
#include "drivers/keys_esp_io_expander.h"

#include "boards/p4/board_p4_sdmmc.h"
#include "drivers/battery_dummy.h"
#include "drivers/keys_generic.h"

namespace Boards::esp32::p4 {
class olimex_p4_devkit : public esp32p4_sdmmc {
  public:
    size_t MemorySize() override { return 32*1024*1024; };

    olimex_p4_devkit();
    ~olimex_p4_devkit() override = default;

    hal::battery::Battery *GetBattery() override;
    hal::touch::Touch *GetTouch() override;
    hal::keys::Keys *GetKeys() override;
    hal::display::Display *GetDisplay() override;
    hal::backlight::Backlight *GetBacklight() override;

    BoardPower PowerState() override;
    bool StorageReady() override;
    void *TurboBuffer() override { return buffer; };
    const char *Name() override;
    void LateInit() override;
    WakeupSource BootReason() override;
    void DebugInfo() override;

  protected:
    i2c_master_bus_handle_t bus_handle = nullptr;
    hal::keys::esp32s3::keys_gpio *_keys = nullptr;
    hal::display::esp32s3::display_dummy *_display = nullptr;
    hal::backlight::esp32s3::backlight_dummy *_backlight = nullptr;
    hal::touch::esp32s3::touch_ft5x06 *_touch = nullptr;
    bool _usbConnected = false;
    hal::battery::esp32s3::battery_dummy *_battery = nullptr;

    void *buffer = nullptr;
    esp_io_expander_handle_t _io_expander = nullptr;
    hal::gpio::Gpio *_cs = nullptr;
    hal::gpio::Gpio *_card_detect = nullptr;
};
}