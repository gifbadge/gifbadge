#pragma once

#include <utility>
#include "hal/display.h"

class display_gc9a01 : public Display {
 public:
  display_gc9a01(int mosi, int sck, int cs, int dc, int reset);
  ~display_gc9a01() override = default;

  esp_lcd_panel_handle_t getPanelHandle() override;
  std::pair<int16_t, int16_t> getResolution() override { return {240, 240}; };
  bool onColorTransDone(esp_lcd_panel_io_color_trans_done_cb_t, void *) override;
  uint8_t *getBuffer() override;
  uint8_t *getBuffer2() override;
  void write(int x_start, int y_start, int x_end, int y_end, const void *color_data) override;
  void write_from_buffer() override;
  bool directRender() override { return false; };
 private:
  esp_lcd_panel_handle_t panel_handle = nullptr;
  esp_lcd_panel_io_handle_t io_handle = nullptr;
};