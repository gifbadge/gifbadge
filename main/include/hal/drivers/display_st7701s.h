#pragma once

#include "hal/display.h"


class display_st7701s: public Display{
public:
    display_st7701s();
    ~display_st7701s() override = default;

    esp_lcd_panel_handle_t getPanelHandle() override;
    esp_lcd_panel_io_handle_t getIoHandle() override;
private:
    esp_lcd_panel_handle_t panel_handle;
    esp_lcd_panel_io_handle_t io_handle;
};