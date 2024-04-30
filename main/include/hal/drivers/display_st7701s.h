#pragma once

#include "hal/display.h"
#include "esp_lcd_panel_io_additions.h"
#include <array>

class display_st7701s : public Display {
 public:
  display_st7701s(spi_line_config_t line_cfg,
                  int hsync,
                  int vsync,
                  int de,
                  int pclk,
                  std::array<int, 16> &rgb);
  ~display_st7701s() override = default;

  esp_lcd_panel_handle_t getPanelHandle() override;
  std::pair<int, int> getResolution() override { return {480, 480}; };
  bool onColorTransDone(esp_lcd_panel_io_color_trans_done_cb_t, void *) override;
  uint8_t *getBuffer() override;
  uint8_t *getBuffer2() override;
  void write(int x_start, int y_start, int x_end, int y_end, const void *color_data) override;
  void write_from_buffer() override;
  bool directRender() override { return true; };
 private:
  size_t fb_number = 2;
  esp_lcd_panel_handle_t panel_handle = nullptr;
  void *_fb0 = nullptr;
  void *_fb1 = nullptr;
};