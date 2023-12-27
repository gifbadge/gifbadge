#pragma once

#include <esp_lcd_panel_io.h>

class Display {
public:
    Display() = default;
    virtual ~Display() = default;

    virtual esp_lcd_panel_handle_t getPanelHandle() = 0;
    virtual esp_lcd_panel_io_handle_t getIoHandle() = 0;
};