#pragma once

#include "../../display.h"
#include "../../../../../managed_components/espressif__esp_lcd_panel_io_additions/include/esp_lcd_panel_io_additions.h"
#include "../../../../../../.espressif/tools/xtensa-esp-elf/esp-13.2.0_20230928/xtensa-esp-elf/xtensa-esp-elf/include/c++/13.2.0/array"

class display_st7701s : public Display {
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
 private:
  size_t fb_number = 2;
  esp_lcd_panel_handle_t panel_handle = nullptr;
  void *_fb0 = nullptr;
  void *_fb1 = nullptr;
  esp_timer_handle_t flushTimerHandle = nullptr;

  flushCallback_t flushCallback = nullptr;

};