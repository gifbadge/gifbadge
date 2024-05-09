#pragma once

#include <utility>
#include "hal/display.h"

class display_gc9a01 : public Display {
 public:
  display_gc9a01(int mosi, int sck, int cs, int dc, int reset);
  ~display_gc9a01() override = default;

  esp_lcd_panel_handle_t getPanelHandle() override;
  bool onColorTransDone(esp_lcd_panel_io_color_trans_done_cb_t, void *) override;
  void write(int x_start, int y_start, int x_end, int y_end, const void *color_data) override;
  bool directRender() override { return false; };
 private:
  esp_lcd_panel_handle_t panel_handle = nullptr;
  esp_lcd_panel_io_handle_t io_handle = nullptr;
};