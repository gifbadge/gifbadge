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

static void clear_screen(const std::shared_ptr<Display> &display, uint8_t *pBuf) {
    if(pBuf != nullptr) {
        memset(pBuf, 255, H_RES * V_RES * 2);
        display->write(0, 0, H_RES, V_RES, pBuf);
    }
}



static void display_usb_logo(const std::shared_ptr<Display> &display, uint8_t *fBuf) {
    ESP_LOGI(TAG, "Displaying USB LOGO");
    clear_screen(display, fBuf);
    auto *png = new PNGImage();
    png->open(usb_png, sizeof(usb_png));
    uint8_t *pBuf = static_cast<uint8_t *>(malloc(png->size().first*png->size().second*2));
    png->loop(pBuf);
    display->write(
                              (H_RES / 2) - (png->size().first / 2),
                              (V_RES / 2) - (png->size().second / 2),
                              (H_RES / 2) + ((png->size().first  + 1) / 2),
                              (V_RES / 2) + ((png->size().second + 1) / 2),
                              pBuf);
    delete png;
    free(pBuf);
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

static void display_err(const std::shared_ptr<Display>& display, uint8_t *pGIFBuf, const char *err) {
    ESP_LOGI(TAG, "Displaying Error");
    clear_screen(display, pGIFBuf);
    render_text_centered(H_RES, V_RES, 10, err, pGIFBuf);
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


static void display_image_batt(const std::shared_ptr<Display> &display, uint8_t *fBuf) {
    ESP_LOGI(TAG, "Displaying Low Battery");
    clear_screen(display, fBuf);
    auto *png = new PNGImage();
    png->open(low_batt_png, sizeof(low_batt_png));
    uint8_t *pBuf = static_cast<uint8_t *>(malloc(png->size().first*png->size().second*2));
    png->loop(pBuf);
    display->write(
                              (H_RES / 2) - (png->size().first / 2),
                              (V_RES / 2) - (png->size().second / 2),
                              (H_RES / 2) + ((png->size().first + 1) / 2),
                              (V_RES / 2) + ((png->size().second + 1) / 2),
                              pBuf);
    delete png;
    free(pBuf);
}

Image *display_file(ImageFactory factory, const char *path, uint8_t *pGIFBuf, const std::shared_ptr<Display>& display) {
    Image *in = factory.create(path);
    if (in) {
        if(in->open(path) != 0){
            std::string err = "Error Displaying File\n";
            err = err + path +"\n" + in->getLastError();
            display_err(display, pGIFBuf, err.c_str() );
            delete in;
            return nullptr;
        }
        printf("%s x: %i y: %i\n", path, in->size().first, in->size().second);
        auto size = in->size();
        if (size.first > H_RES || size.second > V_RES) {
            delete in;
            display_image_too_large(display, pGIFBuf, path);
            return nullptr;
        }
        int delay;
        if (size.first == H_RES && size.second == V_RES) {
            delay = in->loop(pGIFBuf);
            display->write(
                    (H_RES / 2) - (in->size().first / 2),
                    (V_RES / 2) - (in->size().second / 2),
                    (H_RES / 2) + (in->size().first / 2),
                    (V_RES / 2) + (in->size().second / 2),
                    pGIFBuf);
        } else {
            clear_screen(display, pGIFBuf); //Only need to clear the screen if the image won't fill it
            delay = 0;
            auto *pBuf = static_cast<uint8_t *>(malloc(size.first * size.second * 2));
            delay = in->loop(pBuf);
            display->write(
                    (H_RES / 2) - (in->size().first / 2),
                    (V_RES / 2) - (in->size().second / 2),
                    (H_RES / 2) + (in->size().first / 2),
                    (V_RES / 2) + (in->size().second / 2),
                    pBuf);
            free(pBuf);
        }
        if (in->animated()) {
            vTaskDelay(delay / portTICK_PERIOD_MS);
        }
    }
    return in;
}

static int image_loop(std::shared_ptr<Image> &in, uint8_t *pGIFBuf, const std::shared_ptr<Display>& display) {
    int64_t start = esp_timer_get_time();
    if (in && in->animated()) {
        int delay = in->loop(pGIFBuf);
      if(delay >= 0) {
            display->write((H_RES / 2) - (in->size().first / 2),
                           (V_RES / 2) - (in->size().second / 2),
                           (H_RES / 2) + (in->size().first / 2),
                           (V_RES / 2) + (in->size().second / 2),
                           pGIFBuf);
            ESP_LOGI(TAG, "Frame Display time %lli", (esp_timer_get_time()-start)/1000);
            int calc_delay = delay-static_cast<int>((esp_timer_get_time()-start)/1000);
            ESP_LOGI(TAG, "Frame Delay: %i, calculated delay %i", delay, calc_delay);
            if(calc_delay > 0) {
                vTaskDelay(calc_delay / portTICK_PERIOD_MS);
            }
        }
        else {
            ESP_LOGI(TAG, "Image loop error");
            return -1;
        }
    } else {
        vTaskDelay(200 / portTICK_PERIOD_MS);
        return 0;
    }
    return 0;
}

static std::string get_file(const std::filesystem::path &path) {
    FILE *fp = fopen(path.c_str(), "r");
    if (fp != nullptr) {
        fclose(fp);
        return path;
    }
    return list_directory(path).at(0);
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

static std::filesystem::path files_get_next(const std::filesystem::path &path) {
    auto files = list_directory(path.parent_path());
    int pos = files_get_position(files, path);
    if (pos >= 0) {
        if (pos == files.size()-1) {
            return files.at(0);
        } else {
            return files.at(pos + 1);
        }
    }
    else {
        return files.at(0);
    }
}

static std::filesystem::path files_get_previous(const std::filesystem::path &path) {
    auto files = list_directory(path.parent_path());
    int pos = files_get_position(files, path);
    if (pos >= 0) {
        if (pos == 0) {
            return files.at(files.size()-1);
        } else {
            return files.at(pos - 1);
        }
    }
    else {
        return files.at(0);
    }
}

void display_task(void *params) {
    // TODO: Check image size before trying to display it
    auto *args = (display_task_args *) params;

    auto config = ImageConfig();


    bool menu_state = false;

    H_RES = args->display->getResolution().first;
    V_RES = args->display->getResolution().second;


    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    uint8_t *pGIFBuf = args->display->getBuffer();

    memset(pGIFBuf, 255, H_RES * V_RES * 2);
    args->display->write( 0, 0, H_RES, V_RES, pGIFBuf);

    esp_err_t err;
    int backlight_level;
    auto handle = nvs::open_nvs_handle("settings", NVS_READWRITE, &err);
    err = handle->get_item("backlight", backlight_level);
    if(err == ESP_ERR_NVS_NOT_FOUND){
        backlight_level = 10;
    }
    handle.reset();

    args->backlight->setLevel(backlight_level*10);

    ImageFactory factory;
    std::shared_ptr<Image> in;
    std::vector<std::string> files = list_directory("/data");

    std::filesystem::path current_file;

    DISPLAY_OPTIONS last_mode = DISPLAY_NONE;

    int64_t last_change = esp_timer_get_time();

    ESP_LOGI(TAG, "Display Resolution %ix%i", H_RES, V_RES);

    int current_buffer = 1;
    while (true) {
        uint32_t option;
        if (args->display->directRender()) {
            if (current_buffer != 0) {
                pGIFBuf = args->display->getBuffer();
                current_buffer = 0;
            } else {
                pGIFBuf = args->display->getBuffer2();
                current_buffer = 1;
            }
        }
        xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, 0);
        if(option != DISPLAY_NONE) {
            last_change = esp_timer_get_time();
            switch (option) {
                case DISPLAY_USB:
                    in.reset();
                    ESP_LOGI(TAG, "DISPLAY_USB");
                    menu_state = false;
                    display_usb_logo(args->display, pGIFBuf);
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                    break;
                case DISPLAY_MENU:
                    in.reset();
                    ESP_LOGI(TAG, "DISPLAY_MENU");
                    menu_state = true;
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                    break;
                case DISPLAY_FILE:
                    in.reset();
                    ESP_LOGI(TAG, "DISPLAY_FILE");
                    config.reload();
                    menu_state = false;
                    try {
                        current_file = get_file(config.getPath());
                        in.reset(display_file(factory, current_file.c_str(), pGIFBuf,
                                              args->display));
                    }
                    catch (std::out_of_range &err) {
                        display_no_image(args->display, pGIFBuf);
                    }
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                    break;
                case DISPLAY_NEXT:
                    if (config.getLocked()) {
                        break;
                    }
                    if(list_directory(current_file.parent_path()).size() <= 1){
                        break;
                    }
                    in.reset();
                    try {
                        current_file = files_get_next(current_file);
                        in.reset(display_file(factory, current_file.c_str(), pGIFBuf,
                                              args->display));
                    }
                    catch (std::out_of_range &err) {
                        display_no_image(args->display, pGIFBuf);
                    }
                    break;
                case DISPLAY_PREVIOUS:
                    if (config.getLocked()) {
                        break;
                    }
                    if(list_directory(current_file.parent_path()).size() <= 1){
                        break;
                    }
                    in.reset();
                    try {
                        current_file = files_get_previous(current_file);
                        in.reset(display_file(factory, current_file.c_str(), pGIFBuf,
                                              args->display));
                    }
                    catch (std::out_of_range &err) {
                        display_no_image(args->display, pGIFBuf);
                    }
                    break;
                case DISPLAY_BATT:
                    in.reset();
                    if (last_mode != DISPLAY_BATT) {
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
                    in.reset();
                    last_mode = static_cast<DISPLAY_OPTIONS>(option);
                    display_no_storage(args->display, pGIFBuf);
                default:
                    break;
            }
        } else {
            if (!menu_state) {
                if(last_mode == DISPLAY_FILE && config.getSlideShow() && ((esp_timer_get_time()/1000000)-(last_change/1000000)) > config.getSlideShowTime()){
                    xTaskNotifyIndexed(xTaskGetCurrentTaskHandle(), 0, DISPLAY_NEXT, eSetValueWithOverwrite );
                } else if(last_mode == DISPLAY_OTA){
                    uint32_t percent;
                    xTaskNotifyWaitIndexed(1, 0, 0xffffffff, &percent, 0);
                    if(percent != 0){
                        display_ota(args->display, pGIFBuf, percent);
                    }
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                } else {
                    if(image_loop(in, pGIFBuf, args->display) != 0){
                        std::string strerr = "Error Displaying File\n";
                        strerr += current_file.string() +"\n" + in->getLastError();
                        display_err(args->display, pGIFBuf, strerr.c_str() );
                        in.reset();
                    }
                }
            } else {
                vTaskDelay(200 / portTICK_PERIOD_MS);
            }
        }
    }
}