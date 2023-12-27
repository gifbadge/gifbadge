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

#include "font_render.h"
#include "file_util.h"


static const char *TAG = "DISPLAY";

int H_RES = 240;
int V_RES = 240;

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