#include "freertos/FreeRTOS.h"

#include <esp_log.h>
#include <cstring>
#include <vector>
#include <algorithm>

#include "display.h"
#include "image.h"
#include "png.h"
#include "images/usb_png.h"
#include "images/low_batt_png.h"

#include <nvs_handle.hpp>
#include <esp_timer.h>

#include "font_render.h"
#include "file_util.h"


static const char *TAG = "DISPLAY";

int H_RES;
int V_RES;

static void clear_screen(const std::shared_ptr<Display>& display, uint8_t *pGIFBuf) {
    memset(pGIFBuf, 255, H_RES * V_RES * 2);
    display->write(0, 0, H_RES, V_RES, pGIFBuf);
}



static void display_usb_logo(const std::shared_ptr<Display>& display, uint8_t *pGIFBuf) {
    ESP_LOGI(TAG, "Displaying USB LOGO");
    clear_screen(display, pGIFBuf);
    auto *png = new PNGImage();
    png->open(usb_png, sizeof(usb_png));
    png->loop(pGIFBuf);
    display->write(
                              (H_RES / 2) - (png->size().first / 2),
                              (V_RES / 2) - (png->size().second / 2),
                              (H_RES / 2) + ((png->size().first  + 1) / 2),
                              (V_RES / 2) + ((png->size().second + 1) / 2),
                              pGIFBuf);
    delete png;
}

static void display_no_image(const std::shared_ptr<Display>& display, uint8_t *pGIFBuf) {
    ESP_LOGI(TAG, "Displaying No Image");
    clear_screen(display, pGIFBuf);
    render_text_centered(H_RES, V_RES, 10, "No Image", pGIFBuf);
    display->write(
                              0,
                              0,
                              H_RES,
                              V_RES,
                              pGIFBuf);
}

static void display_image_too_large(const std::shared_ptr<Display>& display, uint8_t *pGIFBuf, const char *path) {
    ESP_LOGI(TAG, "Displaying Image To Large");
    clear_screen(display, pGIFBuf);
    char tmp[255];
    sprintf(tmp, "Image too Large\n%s", path);
    render_text_centered(H_RES, V_RES, 10, tmp, pGIFBuf);
    display->write(
                              0,
                              0,
                              H_RES,
                              V_RES,
                              pGIFBuf);
}

static void display_ota(const std::shared_ptr<Display>& display, uint8_t *pGIFBuf, uint32_t percent) {
    ESP_LOGI(TAG, "Displaying OTA Status");
    clear_screen(display, pGIFBuf);
    char tmp[255];
    sprintf(tmp, "Update In Progress\n%lu%%", percent);
    render_text_centered(H_RES, V_RES, 10, tmp, pGIFBuf);
    display->write(
                              0,
                              0,
                              H_RES,
                              V_RES,
                              pGIFBuf);
}

static void display_no_storage(const std::shared_ptr<Display>& display, uint8_t *pGIFBuf) {
    ESP_LOGI(TAG, "Displaying No Storage");
    clear_screen(display, pGIFBuf);
    render_text_centered(H_RES, V_RES, 10, "No SDCARD", pGIFBuf);
    display->write(
                              0,
                              0,
                              H_RES,
                              V_RES,
                              pGIFBuf);
}


static void display_image_batt(const std::shared_ptr<Display>& display, uint8_t *pGIFBuf) {
    ESP_LOGI(TAG, "Displaying Low Battery");
    clear_screen(display, pGIFBuf);
    auto *png = new PNGImage();
    png->open(low_batt_png, sizeof(low_batt_png));
    png->loop(pGIFBuf);
    display->write(
                              (H_RES / 2) - (png->size().first / 2),
                              (V_RES / 2) - (png->size().second / 2),
                              (H_RES / 2) + ((png->size().first + 1) / 2),
                              (V_RES / 2) + ((png->size().second + 1) / 2),
                              pGIFBuf);
    delete png;
}

Image *display_file(ImageFactory factory, const char *path, uint8_t *pGIFBuf, const std::shared_ptr<Display>& display) {
    Image *in = factory.create(path);
    clear_screen(display, pGIFBuf);
    if (in) {
        in->open(path);
        printf("%s x: %i y: %i\n", path, in->size().first, in->size().second);
        auto size = in->size();
        if (size.first > H_RES || size.second > V_RES) {
            delete in;
            display_image_too_large(display, pGIFBuf, path);
            return nullptr;
        }
        int delay = in->loop(pGIFBuf);
        display->write(
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

static void image_loop(std::shared_ptr<Image> &in, uint8_t *pGIFBuf, const std::shared_ptr<Display>& display) {
    if (in && in->animated()) {
        int delay = in->loop(pGIFBuf);
        if(delay >= 0) {
            display->write((H_RES / 2) - (in->size().first / 2),
                           (V_RES / 2) - (in->size().second / 2),
                           (H_RES / 2) + (in->size().first / 2),
                           (V_RES / 2) + (in->size().second / 2),
                           pGIFBuf);
//            display.write(
//                                      (H_RES / 2) - (in->size().first / 2),
//                                      (V_RES / 2) - (in->size().second / 2),
//                                      (H_RES / 2) + (in->size().first / 2),
//                                      (V_RES / 2) + (in->size().second / 2),
//                                      pGIFBuf);
            vTaskDelay(delay / portTICK_PERIOD_MS);
        }
        else {
            ESP_LOGI(TAG, "Image loop error");
        }
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

    auto panel_handle = args->display->getPanelHandle();
    auto config = args->image_config;


    bool menu_state = false;

    H_RES = args->display->getResolution().first;
    V_RES = args->display->getResolution().second;


    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    uint8_t *pGIFBuf = static_cast<uint8_t *>(malloc(
            args->display->getResolution().first * args->display->getResolution().second * 2));

    memset(pGIFBuf, 255, H_RES * V_RES * 2);
    args->display->write( 0, 0, H_RES, V_RES, pGIFBuf);

    args->backlight->setLevel(100);

    ImageFactory factory;
    std::shared_ptr<Image> in;
    std::vector<std::string> files = list_directory("/data");

    std::string current_file;

    DISPLAY_OPTIONS last_mode = DISPLAY_NONE;

    int64_t last_change = esp_timer_get_time();

    ESP_LOGI(TAG, "Display Resolution %ix%i", H_RES, V_RES);

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
                    display_usb_logo(args->display, pGIFBuf);
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
                        in.reset(display_file(factory, current_file.c_str(), pGIFBuf, args->display));
                    }
                    catch (std::out_of_range &err) {
                        display_no_image(args->display, pGIFBuf);
                    }
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                    break;
                case DISPLAY_FILE:
                    ESP_LOGI(TAG, "DISPLAY_FILE");
                    menu_state = false;
                    try {
                        current_file = get_file(config->getDirectory(), config->getFile());
                        in.reset(display_file(factory, current_file.c_str(), pGIFBuf,
                                              args->display));
                    }
                    catch (std::out_of_range &err) {
                        display_no_image(args->display, pGIFBuf);
                    }
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                    break;
                case DISPLAY_NEXT:
                    if (config->getLocked()) {
                        break;
                    }
                    current_file = files_get_next(config->getDirectory(), current_file);
                    in.reset(display_file(factory, current_file.c_str(), pGIFBuf,
                                          args->display));
                    break;
                case DISPLAY_PREVIOUS:
                    if (config->getLocked()) {
                        break;
                    }
                    try {
                        current_file = files_get_previous(config->getDirectory(), current_file);
                        in.reset(display_file(factory, current_file.c_str(), pGIFBuf,
                                              args->display));
                    }
                    catch (std::out_of_range &err) {
                        display_no_image(args->display, pGIFBuf);
                    }
                    break;
                case DISPLAY_BATT:
                    if (last_mode != DISPLAY_BATT) {
                        in.reset();
                        display_image_batt(args->display, pGIFBuf);
                    }
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                    break;
                case DISPLAY_OTA:
                    in.reset();
                    display_ota(args->display, pGIFBuf, 0);
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                    break;
                case DISPLAY_NO_STORAGE:
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                    display_no_storage(args->display, pGIFBuf);
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
                        display_ota(args->display, pGIFBuf, percent);
                    }
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                } else {
                    image_loop(in, pGIFBuf, args->display);
                }
            } else {
                vTaskDelay(200 / portTICK_PERIOD_MS);
            }
        }
    }
}