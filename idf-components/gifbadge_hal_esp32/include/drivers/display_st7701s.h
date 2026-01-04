/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <hal/display.h>
#include "esp_lcd_panel_io_additions.h"
#include <array>

namespace hal::display::esp32s3 {
class display_st7701s : public hal::display::Display {
 public:
  display_st7701s(spi_line_config_t line_cfg,
                  int hsync,
                  int vsync,
                  int de,
                  int pclk,
                  std::array<int, 16> &rgb);
  ~display_st7701s() override = default;

  bool onColorTransDone(flushCallback_t) override;
  void write(int x_start, int y_start, int x_end, int y_end, const void *color_data) override;
  void clear();
 private:
  size_t fb_number = 2;
  esp_lcd_panel_handle_t panel_handle = nullptr;
  void *_fb0 = nullptr;
  void *_fb1 = nullptr;
  esp_timer_handle_t flushTimerHandle = nullptr;

  flushCallback_t flushCallback = nullptr;
  bool _clear = false;
};
}

