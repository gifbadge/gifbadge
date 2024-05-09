#pragma once

#include <utility>
#include <esp_lcd_panel_io.h>

class Display {
 public:
  Display() = default;
  virtual ~Display() = default;

  virtual esp_lcd_panel_handle_t getPanelHandle() = 0;
  virtual bool onColorTransDone(esp_lcd_panel_io_color_trans_done_cb_t, void *) = 0;
  virtual void write(int x_start, int y_start, int x_end, int y_end, const void *color_data) = 0;
  virtual bool directRender() = 0;

  uint8_t *buffer;
  std::pair<int16_t, int16_t> size;

};