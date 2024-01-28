#pragma once

#include "hal/display.h"
#include <array>



class display_st7701s: public Display{
public:
    display_st7701s(int spi_cs, int spi_scl, int spi_sda, int hsync, int vsync, int de, int pclk, int reset, std::array<int, 16> &rgb);
    ~display_st7701s() override = default;

    esp_lcd_panel_handle_t getPanelHandle() override;
    esp_lcd_panel_io_handle_t getIoHandle() override;
    std::pair<int, int> getResolution() override {return {480, 480};};
    bool onColorTransDone(esp_lcd_panel_io_color_trans_done_cb_t, void *) override {return false;};
    uint8_t *getBuffer() override;
    uint8_t *getBuffer2() override;
    void write(int x_start, int y_start, int x_end, int y_end, const void *color_data) override;
    void write_from_buffer() override;
private:
    size_t fb_number = 2;
    esp_lcd_panel_handle_t panel_handle;
    esp_lcd_panel_io_handle_t io_handle;
    void *_fb0;
    void *_fb1;
};