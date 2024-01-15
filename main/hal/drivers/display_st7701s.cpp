#include <esp_lcd_panel_ops.h>
#include <esp_log.h>
#include "hal/drivers/display_st7701s.h"
#include "esp_lcd_panel_io_additions.h"
#include "esp_lcd_st7701.h"

static const char *TAG = "display_st7701s.cpp";


static const st7701_lcd_init_cmd_t buyadisplaycom[] = {
        {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x10,},                                                 5,  0},
        {0xC0, (uint8_t[]) {0x3b, 0x00,},                                                                   2,  0},
        {0xC1, (uint8_t[]) {0x0b, 0x02,},                                                                   2,  0},
        {0xC2, (uint8_t[]) {0x07, 0x02,},                                                                   2,  0},
        {0xCC, (uint8_t[]) {0x10,},                                                                         1,  0},
        {0xCD, (uint8_t[]) {0x08,},                                                                         1,  0},
        {0xB0, (uint8_t[]) {0x00, 0x11, 0x16, 0x0E, 0x11, 0x06, 0x05, 0x09, 0x08, 0x21, 0x06, 0x13, 0x10, 0x29, 0x31,
                            0x18},                                                                          16, 0},
        {0xB1, (uint8_t[]) {0x00, 0x11, 0x16, 0x0E, 0x11, 0x07, 0x05, 0x09, 0x09, 0x21, 0x05, 0x13, 0x11, 0x2A, 0x31,
                            0x18},                                                                          16, 0},
        {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x11,},                                                 5,  0},
        {0xB0, (uint8_t[]) {0x6D},                                                                          1,  0},
        {0xB1, (uint8_t[]) {0x37},                                                                          1,  0},
        {0xB2, (uint8_t[]) {0x81},                                                                          1,  0},
        {0xB3, (uint8_t[]) {0x80},                                                                          1,  0},
        {0xB5, (uint8_t[]) {0x43},                                                                          1,  0},
        {0xB7, (uint8_t[]) {0x85},                                                                          1,  0},
        {0xB8, (uint8_t[]) {0x20},                                                                          1,  0},
        {0xC1, (uint8_t[]) {0x78},                                                                          1,  0},
        {0xC2, (uint8_t[]) {0x78},                                                                          1,  0},

        {0xC3, (uint8_t[]) {0x8C},                                                                          1,  0},
        {0xD0, (uint8_t[]) {0x88},                                                                          1,  0},
        {0xE0, (uint8_t[]) {0x00, 0x00, 0x02},                                                              3,  0},
        {0xE1, (uint8_t[]) {0x03, 0xA0, 0x00, 0x00, 0x04},                                                  5,  0},
        {0xA0, (uint8_t[]) {0x00, 0x00, 0x00, 0x20, 0x20},                                                  5,  0},
        {0xE2, (uint8_t[]) {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}, 13, 0},
        {0xE3, (uint8_t[]) {0x00, 0x00, 0x11, 0x00},                                                        4,  0},
        {0xE4, (uint8_t[]) {0x22, 0x00},                                                                    2,  0},
        {0xE5, (uint8_t[]) {0x05, 0xEC, 0xA0, 0xA0, 0x07, 0xEE, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00},                                                                          16, 0},
        {0xE6, (uint8_t[]) {0x00, 0x00, 0x11, 0x00},                                                        4,  0},
        {0xE7, (uint8_t[]) {0x22, 0x00},                                                                    2,  0},
        {0xE8, (uint8_t[]) {0x06, 0xED, 0xA0, 0xA0, 0x08, 0xEF, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00},                                                                          16, 0},
        {0xEB, (uint8_t[]) {0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00},                                      7,  0},
        {0xED, (uint8_t[]) {0xFF, 0xFF, 0xFF, 0xBA, 0x0A, 0xBF, 0x45, 0xFF, 0xFF, 0x54, 0xFB, 0xA0, 0xAB, 0xFF,
                            0xFF},                                                                          15, 0},
        {0xEF, (uint8_t[]) {0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F},                                            6,  0},
        {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x13},                                                  5,  0},
        {0xEF, (uint8_t[]) {0x08},                                                                          1,  0},
        {0xFF, (uint8_t[]) {0x77, 0x01, 0x00, 0x00, 0x00},                                                  5,  0},
        {0x11, (uint8_t[]) {},                                                                              0,  120},
        {0x29, (uint8_t[]) {},                                                                              0,  0},
//        {0x23, (uint8_t[]) {},                                                                              0,  0},

};

//#define B1 5
//#define B2 4
//#define B3 3
//#define B4 2
//#define B5 1
//#define G0 12
//#define G1 11
//#define G2 9
//#define G3 8
//#define G4 7
//#define G5 6
//#define R1 17
//#define R2 16
//#define R3 15
//#define R4 14
//#define R5 13

display_st7701s::display_st7701s(int spi_cs, int spi_scl, int spi_sda, int hsync, int vsync, int de, int pclk, int reset, std::array<int,16> &rgb /*G3, G4, G5, R1, R2, R3, R4, R5, B1, B2, B3, B4, B5, G0, G1, G2 */) {


    ESP_LOGI(TAG, "Install 3-wire SPI panel IO");
//    spi_line_config_t line_config = {
//            .cs_io_type = IO_TYPE_GPIO,             // Set to `IO_TYPE_GPIO` if using GPIO, same to below
//            .cs_gpio_num = spi_cs,
//            .scl_io_type = IO_TYPE_GPIO,
//            .scl_gpio_num = spi_scl,
//            .sda_io_type = IO_TYPE_GPIO,
//            .sda_gpio_num = spi_sda,
//            .io_expander = nullptr,                        // Set to NULL if not using IO expander
//    };
//    esp_lcd_panel_io_3wire_spi_config_t io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);

    esp_lcd_panel_io_3wire_spi_config_t io_config =    {
        .line_config = {
                .cs_io_type = IO_TYPE_GPIO,             // Set to `IO_TYPE_GPIO` if using GPIO, same to below
                .cs_gpio_num = spi_cs,
                .scl_io_type = IO_TYPE_GPIO,
                .scl_gpio_num = spi_scl,
                .sda_io_type = IO_TYPE_GPIO,
                .sda_gpio_num = spi_sda,
                .io_expander = nullptr,                        // Set to NULL if not using IO expander
        },
        .expect_clk_speed = 100*1000,
        .spi_mode = 0,
        .lcd_cmd_bytes = 1,
        .lcd_param_bytes = 1,
        .flags = {
            .use_dc_bit = 1,
            .dc_zero_on_data = 0,
            .lsb_first = 0,
            .cs_high_active = 0,
            .del_keep_cs_inactive = 1,
        },
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &io_handle));

    ESP_LOGI(TAG, "Install ST7701S panel driver");
    esp_lcd_rgb_panel_config_t rgb_config = {
            .clk_src = LCD_CLK_SRC_XTAL,
//            .clk_src = LCD_CLK_SRC_DEFAULT,
            .timings = {
                    .pclk_hz = 10 * 1000 * 1000,
                    .h_res = 480,
                    .v_res = 480,
                    .hsync_pulse_width = 6,
                    .hsync_back_porch = 18,
                    .hsync_front_porch = 24,
                    .vsync_pulse_width = 4,
                    .vsync_back_porch = 10,
                    .vsync_front_porch = 16,
                    .flags {
                            .hsync_idle_low = 0,
                            .vsync_idle_low = 0,
                            .de_idle_high = 0,
                            .pclk_active_neg = false,
                            .pclk_idle_high = false,}
            },

            .data_width = 16,
            .bits_per_pixel = 16,
            .num_fbs = 1,
            .bounce_buffer_size_px = 0, //10*H_RES,
            .psram_trans_align = 64,
            .hsync_gpio_num = hsync,
            .vsync_gpio_num = vsync,
            .de_gpio_num = de,
            .pclk_gpio_num = pclk,
            .disp_gpio_num = -1,
//            .data_gpio_nums = { B1, B2, B3, B4, B5, G0, G1, G2, G3, G4, G5, R1, R2, R3, R4, R5}, //Working BE
//            .data_gpio_nums = {G3, G4, G5, R1, R2, R3, R4, R5, B1, B2, B3, B4, B5, G0, G1, G2}, //Working LE
            .data_gpio_nums = {rgb[0], rgb[1], rgb[2], rgb[3], rgb[4], rgb[5], rgb[6], rgb[7], rgb[8], rgb[9], rgb[10], rgb[11], rgb[12], rgb[13], rgb[14], rgb[15]},



            .flags = {
                    .fb_in_psram = 1,
            }
    };
    st7701_vendor_config_t vendor_config = {
            .rgb_config = &rgb_config,
            .init_cmds = buyadisplaycom,
            .init_cmds_size = sizeof(buyadisplaycom) / sizeof(st7701_lcd_init_cmd_t),
            .flags = {
                    .auto_del_panel_io = 1,
            },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = reset,
            .rgb_endian = LCD_RGB_ENDIAN_BGR,
            .bits_per_pixel = 18,
            .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(io_handle, &panel_config, &panel_handle));
//    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
//    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
//    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
//    esp_lcd_rgb_panel_set_yuv_conversion(panel_handle, nullptr);
}

esp_lcd_panel_handle_t display_st7701s::getPanelHandle() {
    return panel_handle;
}

esp_lcd_panel_io_handle_t display_st7701s::getIoHandle() {
    return io_handle;
}


