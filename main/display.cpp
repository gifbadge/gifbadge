#include "freertos/FreeRTOS.h"

#include <esp_log.h>
#include <cstring>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include "driver/gpio.h"
#include "grad.h"

#include "display.h"
#include "image.h"
#include "usb_image.h"

#ifdef CONFIG_GC9A01
#include "esp_lcd_gc9a01.h"
#include "driver/spi_master.h"

#endif

#ifdef CONFIG_ST7706

#include "esp_lcd_st7701.h"
#include "esp_lcd_panel_io_additions.h"

#endif


static const char *TAG = "DISPLAY";

#ifdef CONFIG_GC9A01
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
}
#endif

#ifdef CONFIG_ST7706
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
//            .clk_src = LCD_CLK_SRC_XTAL,
            .clk_src = LCD_CLK_SRC_DEFAULT,
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
                            .pclk_active_neg = true,
                            .pclk_idle_high = false,}
            },

            .data_width = 16,
            .bits_per_pixel = 16,
            .num_fbs = 1,
            .bounce_buffer_size_px = 10*H_RES,
            .psram_trans_align = 64,
            .hsync_gpio_num = 48,
            .vsync_gpio_num = 47,
            .de_gpio_num = 33,
            .pclk_gpio_num = 37,
            .disp_gpio_num = -1,
            .data_gpio_nums = { B1, B2, B3, B4, B5, G0, G1, G2, G3, G4, G5, R1, R2, R3, R4, R5},
            .flags = {
                    .fb_in_psram = 1,
                    }
    };
    st7701_vendor_config_t vendor_config = {
            .rgb_config = &rgb_config,
            .init_cmds = buyadisplaycom,      // Uncomment these line if use custom initialization commands
            .init_cmds_size = sizeof(buyadisplaycom) / sizeof(st7701_lcd_init_cmd_t),
            .flags = {
                    .auto_del_panel_io = 0,
            },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = 46,           // Set to -1 if not use
            .rgb_endian = LCD_RGB_ENDIAN_BGR,     // Implemented by LCD command `36h`
            .bits_per_pixel = 18,    // Implemented by LCD command `3Ah` (16/18/24)
            .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(*io_handle, &panel_config, panel_handle));    /**
                                                                                             * Only create RGB when `auto_del_panel_io` is set to 0,
                                                                                             * or initialize st7701 meanwhile
                                                                                             */
    ESP_ERROR_CHECK(esp_lcd_panel_reset(*panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*panel_handle));
    esp_lcd_rgb_panel_set_yuv_conversion(*panel_handle, nullptr);
}

#endif


void lcd_init(esp_lcd_panel_handle_t *panel_handle, esp_lcd_panel_io_handle_t *io_handle, void *user_ctx,
              esp_lcd_panel_io_color_trans_done_cb_t callback) {
#ifdef CONFIG_LCD_1_28_GC9A01
    lcd_gc9a01(panel_handle, io_handle, user_ctx, callback);
#endif
#ifdef CONFIG_LCD_2_1_ST7706
    lcd_st7706(panel_handle, io_handle, user_ctx, callback);
#endif
}

std::vector<std::string> list_directory(const std::string &path) {
    std::vector<std::string> files;
    DIR *dir = opendir(path.c_str());
    if (dir != nullptr) {
        while (true) {
            struct dirent *de = readdir(dir);
            if (!de) {
                break;
            }
            struct stat statbuf{};
            if (stat((path + "/" + de->d_name).c_str(), &statbuf) == 0) {
                if (!S_ISDIR(statbuf.st_mode)) {
                    files.emplace_back(path + "/" + de->d_name);
                }
            }
        }
        closedir(dir);
    }
    return files;
}

Image *display_file(ImageFactory factory, const char *path, uint8_t *pGIFBuf, esp_lcd_panel_handle_t panel_handle) {
    Image *in = factory.create(path);
    if (in) {
        in->open(path);
        printf("%s x: %i y: %i\n", path, in->size().first, in->size().second);
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

static void display_usb_logo(esp_lcd_panel_handle_t panel_handle, uint8_t *pGIFBuf) {
    ESP_LOGI(TAG, "Displaying USB LOGO");
    memcpy(pGIFBuf, USB_icon,
           USB_ICON_HEIGHT * USB_ICON_WIDTH * 2); //Why I need to copy this into memory, I am not sure
    esp_lcd_panel_draw_bitmap(panel_handle,
                              (H_RES / 2) - (USB_ICON_WIDTH / 2),
                              (V_RES / 2) - (USB_ICON_HEIGHT / 2),
                              (H_RES / 2) + (USB_ICON_WIDTH / 2),
                              (V_RES / 2) + (USB_ICON_HEIGHT / 2),
                              pGIFBuf);
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

static void clear_screen(esp_lcd_panel_handle_t panel_handle, uint8_t *pGIFBuf) {
    memset(pGIFBuf, 255, 240 * 240 * 2);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 240, 240, pGIFBuf);
}

void display_task(void *params) {
    auto *args = (display_task_args *) params;

    auto panel_handle = (esp_lcd_panel_handle_t) args->panel_handle;
    auto config = args->image_config;


    bool menu_state = false;


    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    uint8_t *pGIFBuf;
    pGIFBuf = (uint8_t *) heap_caps_malloc(H_RES * V_RES * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);//MALLOC_CAP_DMA);
//    panel_st7701_get_frame_buffer(panel_handle, 1, (void **) &pGIFBuf);
    memset(pGIFBuf, 255, H_RES * V_RES * 2);
//    memcpy(pGIFBuf, _grad,
//           480 * 480 * 2);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, H_RES, V_RES, pGIFBuf);
//    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level((gpio_num_t) PIN_NUM_BK_LIGHT, LCD_BK_LIGHT_ON_LEVEL);

    ImageFactory factory;
    std::shared_ptr<Image> in;
    std::vector<std::string> files = list_directory("/data");

    while (true) {
        uint32_t option;
        xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, 0);
        switch (option) {
            case DISPLAY_USB:
                ESP_LOGI(TAG, "DISPLAY_USB");
                menu_state = false;
                in.reset();
                display_usb_logo(panel_handle, pGIFBuf);
                break;
            case DISPLAY_MENU:
                ESP_LOGI(TAG, "DISPLAY_MENU");
                menu_state = true;
                break;
            case DISPLAY_PATH:
                ESP_LOGI(TAG, "DISPLAY_PATH");
                clear_screen(panel_handle, pGIFBuf);
                files = list_directory(config->getDirectory());
                in.reset(display_file(factory, files.at(0).c_str(), pGIFBuf, panel_handle));
                break;
            case DISPLAY_FILE:
                ESP_LOGI(TAG, "DISPLAY_FILE");
                clear_screen(panel_handle, pGIFBuf);
                menu_state = false;
                in.reset(display_file(factory, get_file(config->getDirectory(), config->getFile()).c_str(), pGIFBuf,
                                      panel_handle));
                break;
            default:
            case DISPLAY_NONE:
                if (!menu_state) {
                    image_loop(in, pGIFBuf, panel_handle);
                } else {
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                }
        }
    }
}