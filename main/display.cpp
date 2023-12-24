#include "freertos/FreeRTOS.h"

#include <esp_log.h>
#include <cstring>
#include <vector>
#include "driver/gpio.h"
#include <algorithm>

#include "display.h"
#include "image.h"
#include "images/usb_image.h"
#include "images/low_batt.h"

#include <nvs_handle.hpp>
#include <esp_timer.h>
#include <driver/ledc.h>


#include "esp_lcd_gc9a01.h"
#include "driver/spi_master.h"

#include "esp_lcd_st7701.h"
#include "esp_lcd_panel_io_additions.h"

#include "font_render.h"
#include "file_util.h"


static const char *TAG = "DISPLAY";

int H_RES;
int V_RES;


void lcd_gc9a01(esp_lcd_panel_handle_t *panel_handle, esp_lcd_panel_io_handle_t *io_handle, void *user_ctx,
                esp_lcd_panel_io_color_trans_done_cb_t callback) {
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
            .pin_bit_mask = 1ULL << (gpio_num_t) PIN_NUM_BK_LIGHT,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
            .mosi_io_num = 35,//21,
            .miso_io_num = -1,
            .sclk_io_num = 36,//40,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .data4_io_num = -1,
            .data5_io_num = -1,
            .data6_io_num = -1,
            .data7_io_num = -1,
            .max_transfer_sz = 32768, //H_RES * V_RES * sizeof(uint16_t),
            .flags = 0,
            .isr_cpu_id = INTR_CPU_ID_AUTO,
            .intr_flags = 0,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_spi_config_t io_config = {
            .cs_gpio_num = 34, //47,
            .dc_gpio_num = 37,//45,
            .spi_mode = 0,
            .pclk_hz = (80 * 1000 * 1000),
            .trans_queue_depth = 10,
            .on_color_trans_done = callback,
            .user_ctx = user_ctx,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .flags = {
                    .dc_low_on_data = 0,
                    .octal_mode = 0,
                    .sio_mode = 0,
                    .lsb_first = 0,
                    .cs_high_active = 0,
            }
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) SPI2_HOST, &io_config, io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = 46,
            .rgb_endian = LCD_RGB_ENDIAN_BGR,
            .bits_per_pixel = 16,
            .flags = {
                    .reset_active_high = 0,
            },
            .vendor_config = nullptr,
    };

    ESP_LOGI(TAG, "Install GC9A01 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(*io_handle, &panel_config, panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(*panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(*panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(*panel_handle, true, false));
    gpio_hold_en((gpio_num_t) 46); //Don't toggle the reset signal on light sleep
}


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
};

#define B1 5
#define B2 4
#define B3 3
#define B4 2
#define B5 1
#define G0 12
#define G1 11
#define G2 9
#define G3 8
#define G4 7
#define G5 6
#define R1 17
#define R2 16
#define R3 15
#define R4 14
#define R5 13




void lcd_st7706(esp_lcd_panel_handle_t *panel_handle, esp_lcd_panel_io_handle_t *io_handle, void *user_ctx,
                esp_lcd_panel_io_color_trans_done_cb_t callback) {
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
            .pin_bit_mask = 1ULL << (gpio_num_t) PIN_NUM_BK_LIGHT,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level((gpio_num_t) PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);

    ESP_LOGI(TAG, "Install 3-wire SPI panel IO");
    spi_line_config_t line_config = {
            .cs_io_type = IO_TYPE_GPIO,             // Set to `IO_TYPE_GPIO` if using GPIO, same to below
            .cs_gpio_num = 34,
            .scl_io_type = IO_TYPE_GPIO,
            .scl_gpio_num = 36,
            .sda_io_type = IO_TYPE_GPIO,
            .sda_gpio_num = 35,
            .io_expander = nullptr,                        // Set to NULL if not using IO expander
    };
    esp_lcd_panel_io_3wire_spi_config_t io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, io_handle));

    ESP_LOGI(TAG, "Install ST7701S panel driver");
    esp_lcd_rgb_panel_config_t rgb_config = {
            .clk_src = LCD_CLK_SRC_XTAL,
//            .clk_src = LCD_CLK_SRC_DEFAULT,
            .timings = {
                    .pclk_hz = 6 * 1000 * 1000,
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
                            .pclk_active_neg = true,
                            .pclk_idle_high = false,}
            },

            .data_width = 16,
            .bits_per_pixel = 16,
            .num_fbs = 1,
            .bounce_buffer_size_px = 0, //10*H_RES,
            .psram_trans_align = 64,
            .hsync_gpio_num = 48,
            .vsync_gpio_num = 47,
            .de_gpio_num = 33,
            .pclk_gpio_num = 37,
            .disp_gpio_num = -1,
//            .data_gpio_nums = { B1, B2, B3, B4, B5, G0, G1, G2, G3, G4, G5, R1, R2, R3, R4, R5}, //Working BE
            .data_gpio_nums = { G3, G4, G5, R1, R2, R3, R4, R5, B1, B2, B3, B4, B5, G0, G1, G2}, //Working LE



            .flags = {
                    .fb_in_psram = 1,
                    }
    };
    st7701_vendor_config_t vendor_config = {
            .rgb_config = &rgb_config,
            .init_cmds = buyadisplaycom,
            .init_cmds_size = sizeof(buyadisplaycom) / sizeof(st7701_lcd_init_cmd_t),
            .flags = {
                    .auto_del_panel_io = 0,
            },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = 46,
            .rgb_endian = LCD_RGB_ENDIAN_BGR,
            .bits_per_pixel = 18,
            .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(*io_handle, &panel_config, panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(*panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(*panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(*panel_handle, true, false));
    esp_lcd_rgb_panel_set_yuv_conversion(*panel_handle, nullptr);
}

void lcd_init(esp_lcd_panel_handle_t *panel_handle, esp_lcd_panel_io_handle_t *io_handle, void *user_ctx,
              esp_lcd_panel_io_color_trans_done_cb_t callback) {
    esp_err_t err;
    std::unique_ptr<nvs::NVSHandle> handle = nvs::open_nvs_handle("display", NVS_READWRITE, &err);
    DISPLAY_TYPES display_type = DISPLAY_TYPE_NONE;
    err = handle->get_item("type", display_type);
    switch (1) {
        case DISPLAY_TYPE_NONE:
            break;
        case DISPLAY_TYPE_GC9A01_1_28:
            H_RES = 240;
            V_RES = 240;
            lcd_gc9a01(panel_handle, io_handle, user_ctx, callback);
            break;
        case DISPLAY_TYPE_ST7706_2_1:
            H_RES = 480;
            V_RES = 480;
            lcd_st7706(panel_handle, io_handle, user_ctx, callback);
            break;
    }
}

static void clear_screen(esp_lcd_panel_handle_t panel_handle, uint8_t *pGIFBuf) {
    memset(pGIFBuf, 255, H_RES * V_RES * 2);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, H_RES, V_RES, pGIFBuf);
}



static void display_usb_logo(esp_lcd_panel_handle_t panel_handle, uint8_t *pGIFBuf) {
    ESP_LOGI(TAG, "Displaying USB LOGO");
    clear_screen(panel_handle, pGIFBuf);
    esp_lcd_panel_draw_bitmap(panel_handle,
                              (H_RES / 2) - (USB_ICON_WIDTH / 2),
                              (V_RES / 2) - (USB_ICON_HEIGHT / 2),
                              (H_RES / 2) + (USB_ICON_WIDTH / 2),
                              (V_RES / 2) + (USB_ICON_HEIGHT / 2),
                              USB_icon);
}

static void display_no_image(esp_lcd_panel_handle_t panel_handle, uint8_t *pGIFBuf) {
    ESP_LOGI(TAG, "Displaying No Image");
    clear_screen(panel_handle, pGIFBuf);
    render_text_centered(H_RES, V_RES, 10, "No Image", pGIFBuf);
    esp_lcd_panel_draw_bitmap(panel_handle,
                              0,
                              0,
                              H_RES,
                              V_RES,
                              pGIFBuf);
}

static void display_image_too_large(esp_lcd_panel_handle_t panel_handle, uint8_t *pGIFBuf, const char *path) {
    ESP_LOGI(TAG, "Displaying Image To Large");
    clear_screen(panel_handle, pGIFBuf);
    char tmp[255];
    sprintf(tmp, "Image too Large\n%s", path);
    render_text_centered(H_RES, V_RES, 10, tmp, pGIFBuf);
    esp_lcd_panel_draw_bitmap(panel_handle,
                              0,
                              0,
                              H_RES,
                              V_RES,
                              pGIFBuf);
}

static void display_ota(esp_lcd_panel_handle_t panel_handle, uint8_t *pGIFBuf, uint32_t percent) {
    ESP_LOGI(TAG, "Displaying OTA Status");
    clear_screen(panel_handle, pGIFBuf);
    char tmp[255];
    sprintf(tmp, "Update In Progress\n%lu%%", percent);
    render_text_centered(H_RES, V_RES, 10, tmp, pGIFBuf);
    esp_lcd_panel_draw_bitmap(panel_handle,
                              0,
                              0,
                              H_RES,
                              V_RES,
                              pGIFBuf);
}


static void display_image_batt(esp_lcd_panel_handle_t panel_handle, uint8_t *pGIFBuf) {
    ESP_LOGI(TAG, "Displaying Image To Large");
    clear_screen(panel_handle, pGIFBuf);
    esp_lcd_panel_draw_bitmap(panel_handle,
                              (H_RES / 2) - (LOW_BATT_WIDTH / 2),
                              (V_RES / 2) - (LOW_BATT_HEIGHT / 2),
                              (H_RES / 2) + ((LOW_BATT_WIDTH + 1) / 2),
                              (V_RES / 2) + ((LOW_BATT_HEIGHT + 1) / 2),
                              low_batt);
}

Image *display_file(ImageFactory factory, const char *path, uint8_t *pGIFBuf, esp_lcd_panel_handle_t panel_handle) {
    Image *in = factory.create(path);
    clear_screen(panel_handle, pGIFBuf);
    if (in) {
        in->open(path);
        printf("%s x: %i y: %i\n", path, in->size().first, in->size().second);
        auto size = in->size();
        if (size.first > H_RES || size.second > V_RES) {
            delete in;
            display_image_too_large(panel_handle, pGIFBuf, path);
            return nullptr;
        }
        int delay = in->loop(pGIFBuf);
        esp_lcd_panel_draw_bitmap(panel_handle,
                                  (H_RES / 2) - (in->size().first / 2),
                                  (V_RES / 2) - (in->size().second / 2),
                                  (H_RES / 2) + (in->size().first / 2),
                                  (V_RES / 2) + (in->size().second / 2),
                                  pGIFBuf);
        if (in->animated()) {
            vTaskDelay(delay / portTICK_PERIOD_MS);
        }
    }
    return in;
}

static void image_loop(std::shared_ptr<Image> &in, uint8_t *pGIFBuf, esp_lcd_panel_handle_t panel_handle) {
    if (in && in->animated()) {
        int delay = in->loop(pGIFBuf);
        esp_lcd_panel_draw_bitmap(panel_handle,
                                  (H_RES / 2) - (in->size().first / 2),
                                  (V_RES / 2) - (in->size().second / 2),
                                  (H_RES / 2) + (in->size().first / 2),
                                  (V_RES / 2) + (in->size().second / 2),
                                  pGIFBuf);
        vTaskDelay(delay / portTICK_PERIOD_MS);
    } else {
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

static std::string get_file(const std::string &directory, const std::string &f) {
    std::string path = directory + "/" + f;
    FILE *fp = fopen(path.c_str(), "r");
    if (fp != nullptr) {
        fclose(fp);
        return path;
    }
    return list_directory(directory).at(0);
}

static int files_get_position(const std::vector<std::string> &files, const std::string &name) {
//    auto files = list_directory(path);
    auto result = std::find(files.begin(), files.end(), name.c_str());
    if (result == files.end()) {
        return -1;
    } else {
        return std::distance(files.begin(), result);
    }
}

static std::string files_get_next(const std::string &path, const std::string &name) {
    ESP_LOGI(TAG, "%s %s", path.c_str(), name.c_str());
    auto files = list_directory(path);
    for (auto &f: files){
        ESP_LOGI(TAG, "%s", f.c_str());
    }
    int pos = files_get_position(files, name);
    ESP_LOGI(TAG, "%i", pos);
    if (pos >= 0) { //TODO handle error cases
        if (pos == files.size()-1) {
            return files.at(0);
        } else {
            return files.at(pos + 1);
        }
    }
    else {
        return files.at(0);
    }
    return "";
}

static std::string files_get_previous(const std::string &path, const std::string &name) {
    auto files = list_directory(path);
    int pos = files_get_position(files, name);
    if (pos >= 0) { //TODO handle error cases
        if (pos == 0) {
            return files.at(files.size()-1);
        } else {
            return files.at(pos - 1);
        }
    }
    else {
        return files.at(0);
    }
    return "";
}

void display_task(void *params) {
    // TODO: Check image size before trying to display it
    auto *args = (display_task_args *) params;

    auto panel_handle = (esp_lcd_panel_handle_t) args->panel_handle;
    auto config = args->image_config;


    bool menu_state = false;

    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    uint8_t *pGIFBuf;
    pGIFBuf = (uint8_t *) heap_caps_malloc(H_RES * V_RES * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    memset(pGIFBuf, 255, H_RES * V_RES * 2);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, H_RES, V_RES, pGIFBuf);
#ifdef CONFIG_GC9A01
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
#endif

    ESP_LOGI(TAG, "Turn on LCD backlight");
//    gpio_set_level((gpio_num_t) PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);
////    gpio_hold_en((gpio_num_t) PIN_NUM_BK_LIGHT);

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
            .speed_mode       = LEDC_LOW_SPEED_MODE,
            .duty_resolution  = LEDC_TIMER_8_BIT,
            .timer_num        = LEDC_TIMER_0,
            .freq_hz          = 4000,  // Set output frequency at 4 kHz
            .clk_cfg          = LEDC_USE_RC_FAST_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
            .gpio_num       = (gpio_num_t) PIN_NUM_BK_LIGHT,
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = LEDC_CHANNEL_0,
            .intr_type      = LEDC_INTR_DISABLE,
            .timer_sel      = LEDC_TIMER_0,
            .duty           = 0, // Set duty to 0%
            .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 8191);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ESP_ERROR_CHECK(gpio_sleep_sel_dis((gpio_num_t) PIN_NUM_BK_LIGHT));


    ImageFactory factory;
    std::shared_ptr<Image> in;
    std::vector<std::string> files = list_directory("/data");

    std::string current_file;

    DISPLAY_OPTIONS last_mode = DISPLAY_NONE;

    int64_t last_change = esp_timer_get_time();

    while (true) {
        uint32_t option;
        xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, 0);
        if(option != DISPLAY_NONE) {
            last_change = esp_timer_get_time();
            switch (option) {
                case DISPLAY_USB:
                    ESP_LOGI(TAG, "DISPLAY_USB");
                    menu_state = false;
                    in.reset();
                    display_usb_logo(panel_handle, pGIFBuf);
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                    break;
                case DISPLAY_MENU:
                    ESP_LOGI(TAG, "DISPLAY_MENU");
                    menu_state = true;
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                    break;
                case DISPLAY_PATH:
                    ESP_LOGI(TAG, "DISPLAY_PATH");
                    files = list_directory(config->getDirectory()); //TODO: Handle the directory not existing
                    try {
                        current_file = files.at(0);
                        in.reset(display_file(factory, current_file.c_str(), pGIFBuf, panel_handle));
                    }
                    catch (std::out_of_range &err) {
                        display_no_image(panel_handle, pGIFBuf);
                    }
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                    break;
                case DISPLAY_FILE:
                    ESP_LOGI(TAG, "DISPLAY_FILE");
                    menu_state = false;
                    try {
                        current_file = get_file(config->getDirectory(), config->getFile());
                        in.reset(display_file(factory, current_file.c_str(), pGIFBuf,
                                              panel_handle));
                    }
                    catch (std::out_of_range &err) {
                        display_no_image(panel_handle, pGIFBuf);
                    }
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                    break;
                case DISPLAY_NEXT:
                    if (config->getLocked()) {
                        break;
                    }
                    current_file = files_get_next(config->getDirectory(), current_file);
                    in.reset(display_file(factory, current_file.c_str(), pGIFBuf,
                                          panel_handle));
                    break;
                case DISPLAY_PREVIOUS:
                    if (config->getLocked()) {
                        break;
                    }
                    try {
                        current_file = files_get_previous(config->getDirectory(), current_file);
                        in.reset(display_file(factory, current_file.c_str(), pGIFBuf,
                                              panel_handle));
                    }
                    catch (std::out_of_range &err) {
                        display_no_image(panel_handle, pGIFBuf);
                    }
                    break;
                case DISPLAY_BATT:
                    if (last_mode != DISPLAY_BATT) {
                        in.reset();
                        display_image_batt(panel_handle, pGIFBuf);
                    }
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                    break;
                case DISPLAY_OTA:
                    in.reset();
                    display_ota(panel_handle, pGIFBuf, 0);
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                default:
                    break;
            }
        } else {
            if (!menu_state) {
                if((last_mode == DISPLAY_PATH ||last_mode == DISPLAY_FILE) && config->getSlideShow() && ((esp_timer_get_time()/1000000)-(last_change/1000000)) > config->getSlideShowTime()){
                    xTaskNotifyIndexed(xTaskGetCurrentTaskHandle(), 0, DISPLAY_NEXT, eSetValueWithOverwrite );
                } else if(last_mode == DISPLAY_OTA){
                    uint32_t percent;
                    xTaskNotifyWaitIndexed(1, 0, 0xffffffff, &percent, 0);
                    if(percent != 0){
                        display_ota(panel_handle, pGIFBuf, percent);
                    }
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                } else {
                    image_loop(in, pGIFBuf, panel_handle);
                }
            } else {
                vTaskDelay(200 / portTICK_PERIOD_MS);
            }
        }
    }
}