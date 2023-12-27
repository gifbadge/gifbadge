#pragma once

#include "hal/display.h"

class display_gc9a01: public Display{
public:
    display_gc9a01(int mosi, int sck, int cs, int dc, int reset);
    ~display_gc9a01() override = default;

    esp_lcd_panel_handle_t getPanelHandle() override;
    esp_lcd_panel_io_handle_t getIoHandle() override;
private:
    esp_lcd_panel_handle_t panel_handle;
    esp_lcd_panel_io_handle_t io_handle;
};