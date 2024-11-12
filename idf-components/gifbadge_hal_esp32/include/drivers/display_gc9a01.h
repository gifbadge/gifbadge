#pragma once

#include <utility>
#include <hal/display.h>

namespace hal::display::esp32s3 {
class display_gc9a01 : public hal::display::Display {
 public:
  display_gc9a01(int mosi, int sck, int cs, int dc, int reset);
  ~display_gc9a01() override = default;

  bool onColorTransDone(flushCallback_t) override;
  void write(int x_start, int y_start, int x_end, int y_end, const void *color_data) override;
  void clear() override;
 private:
  esp_lcd_panel_handle_t panel_handle = nullptr;
  esp_lcd_panel_io_handle_t io_handle = nullptr;
};
}

