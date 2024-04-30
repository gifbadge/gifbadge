#include "freertos/FreeRTOS.h"

#include <esp_log.h>
#include <cstring>
#include <vector>
#include <algorithm>

#include "display.h"
#include "image.h"
#include "png.h"
#include "images/low_batt_png.h"

#include <nvs_handle.hpp>
#include <esp_timer.h>

#include "font_render.h"
#include "file_util.h"
#include "ui/menu.h"

static const char *TAG = "DISPLAY";

int H_RES;
int V_RES;

static void clear_screen(const std::shared_ptr<Display> &display, uint8_t *pBuf) {
  if (pBuf != nullptr) {
    memset(pBuf, 255, H_RES * V_RES * 2);
    display->write(0, 0, H_RES, V_RES, pBuf);
  }
}

static void display_no_image(const std::shared_ptr<Display> &display, uint8_t *pGIFBuf) {
  ESP_LOGI(TAG, "Displaying No Image");
  clear_screen(display, pGIFBuf);
  render_text_centered(H_RES, V_RES, 10, "No Image", pGIFBuf);
  display->write(0, 0, H_RES, V_RES, pGIFBuf);
}

static void display_image_too_large(const std::shared_ptr<Display> &display, uint8_t *pGIFBuf, const char *path) {
  ESP_LOGI(TAG, "Displaying Image To Large");
  clear_screen(display, pGIFBuf);
  char tmp[255];
  sprintf(tmp, "Image too Large\n%s", path);
  render_text_centered(H_RES, V_RES, 10, tmp, pGIFBuf);
  display->write(0, 0, H_RES, V_RES, pGIFBuf);
}

static void display_err(const std::shared_ptr<Display> &display, uint8_t *pGIFBuf, const char *err) {
  ESP_LOGI(TAG, "Displaying Error");
  clear_screen(display, pGIFBuf);
  render_text_centered(H_RES, V_RES, 10, err, pGIFBuf);
  display->write(0, 0, H_RES, V_RES, pGIFBuf);
}

static void display_ota(const std::shared_ptr<Display> &display, uint8_t *pGIFBuf, uint32_t percent) {
  ESP_LOGI(TAG, "Displaying OTA Status");
  clear_screen(display, pGIFBuf);
  char tmp[50];
  sprintf(tmp, "Update In Progress\n%lu%%", percent);
  render_text_centered(H_RES, V_RES, 10, tmp, pGIFBuf);
  display->write(0, 0, H_RES, V_RES, pGIFBuf);
}

static void display_no_storage(const std::shared_ptr<Display> &display, uint8_t *pGIFBuf) {
  ESP_LOGI(TAG, "Displaying No Storage");
  clear_screen(display, pGIFBuf);
  render_text_centered(H_RES, V_RES, 10, "No SDCARD", pGIFBuf);
  display->write(0, 0, H_RES, V_RES, pGIFBuf);
}

static void display_image_batt(const std::shared_ptr<Display> &display, uint8_t *fBuf) {
  ESP_LOGI(TAG, "Displaying Low Battery");
  clear_screen(display, fBuf);
  auto *png = new PNGImage();
  png->open(low_batt_png, sizeof(low_batt_png));
  uint8_t *pBuf = static_cast<uint8_t *>(heap_caps_malloc(png->size().first * png->size().second * 2, MALLOC_CAP_SPIRAM));
  png->loop(pBuf);
  display->write((H_RES / 2) - (png->size().first / 2),
                 (V_RES / 2) - (png->size().second / 2),
                 (H_RES / 2) + ((png->size().first + 1) / 2),
                 (V_RES / 2) + ((png->size().second + 1) / 2),
                 pBuf);
  delete png;
  free(pBuf);
}

Image *display_file(const std::filesystem::path& path, uint8_t *pGIFBuf, const std::shared_ptr<Display> &display) {
  Image *in = ImageFactory(path.c_str());
  if (in) {
    if (in->open(path.c_str()) != 0) {
      std::string err = "Error Displaying File\n";
      err = err + path.c_str() + "\n" + in->getLastError();
      display_err(display, pGIFBuf, err.c_str());
      delete in;
      return nullptr;
    }
    printf("%s x: %i y: %i\n", path.c_str(), in->size().first, in->size().second);
    auto size = in->size();
    if (size.first > H_RES || size.second > V_RES) {
      delete in;
      display_image_too_large(display, pGIFBuf, path.c_str());
      return nullptr;
    }
    int delay;
    if (size.first == H_RES && size.second == V_RES) {
      delay = in->loop(pGIFBuf);
      display->write((H_RES / 2) - (in->size().first / 2),
                     (V_RES / 2) - (in->size().second / 2),
                     (H_RES / 2) + (in->size().first / 2),
                     (V_RES / 2) + (in->size().second / 2),
                     pGIFBuf);
    } else {
      clear_screen(display, pGIFBuf); //Only need to clear the screen if the image won't fill it
      auto *pBuf = static_cast<uint8_t *>(heap_caps_malloc(size.first * size.second * 2, MALLOC_CAP_SPIRAM));
      delay = in->loop(pBuf);
      display->write((H_RES / 2) - (in->size().first / 2),
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

static int image_loop(std::shared_ptr<Image> &in, uint8_t *pGIFBuf, const std::shared_ptr<Display> &display) {
  int64_t start = esp_timer_get_time();
  if (in && in->animated()) {
    int delay = in->loop(pGIFBuf);
    if (delay >= 0) {
      display->write((H_RES / 2) - (in->size().first / 2),
                     (V_RES / 2) - (in->size().second / 2),
                     (H_RES / 2) + (in->size().first / 2),
                     (V_RES / 2) + (in->size().second / 2),
                     pGIFBuf);
//            ESP_LOGI(TAG, "Frame Display time %lli", (esp_timer_get_time()-start)/1000);
      int calc_delay = delay - static_cast<int>((esp_timer_get_time() - start) / 1000);
//            ESP_LOGI(TAG, "Frame Delay: %i, calculated delay %i", delay, calc_delay);
      return calc_delay > 0 ? calc_delay : 0;
    } else {
      ESP_LOGI(TAG, "Image loop error");
      return -1;
    }
  } else {
    return 2000;
  }
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
    if (pos == files.size() - 1) {
      return files.at(0);
    } else {
      return files.at(pos + 1);
    }
  } else {
    return files.at(0);
  }
}

static std::filesystem::path files_get_previous(const std::filesystem::path &path) {
  auto files = list_directory(path.parent_path());
  int pos = files_get_position(files, path);
  if (pos >= 0) {
    if (pos == 0) {
      return files.at(files.size() - 1);
    } else {
      return files.at(pos - 1);
    }
  } else {
    return files.at(0);
  }
}

void display_task(void *params) {
  // TODO: Check image size before trying to display it
  auto *board = (Board *) params;

  auto config = ImageConfig();

  bool oldMenuState = false;

  H_RES = board->getDisplay()->getResolution().first;
  V_RES = board->getDisplay()->getResolution().second;


  // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
  uint8_t *pGIFBuf = board->getDisplay()->getBuffer();

  memset(pGIFBuf, 255, H_RES * V_RES * 2);
  board->getDisplay()->write(0, 0, H_RES, V_RES, pGIFBuf);

  esp_err_t err;
  int backlight_level;
  auto handle = nvs::open_nvs_handle("settings", NVS_READWRITE, &err);
  err = handle->get_item("backlight", backlight_level);
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    backlight_level = 10;
  }
  handle.reset();

  board->getBacklight()->setLevel(backlight_level * 10);

  std::shared_ptr<Image> in;
  std::vector<std::string> files = list_directory("/data");

  std::filesystem::path current_file;

  DISPLAY_OPTIONS last_mode = DISPLAY_NONE;

  int64_t last_change = esp_timer_get_time();

  ESP_LOGI(TAG, "Display Resolution %ix%i", H_RES, V_RES);

  int current_buffer = 1;
  int delay = 0;
  while (true) {
    uint32_t option;
    if (board->getDisplay()->directRender()) {
      if (current_buffer != 0) {
        pGIFBuf = board->getDisplay()->getBuffer();
        current_buffer = 0;
      } else {
        pGIFBuf = board->getDisplay()->getBuffer2();
        current_buffer = 1;
      }
    }
    xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, delay / portTICK_PERIOD_MS);
    if (option != DISPLAY_NONE) {
      last_change = esp_timer_get_time();
      switch (option) {
        case DISPLAY_FILE:
          in.reset();
          ESP_LOGI(TAG, "DISPLAY_FILE");
          config.reload();
          try {
            if(current_file.empty()) {
              current_file = get_file(config.getPath());
            }
            in.reset(display_file(current_file.c_str(), pGIFBuf, board->getDisplay()));
          } catch (std::out_of_range &err) {
            display_no_image(board->getDisplay(), pGIFBuf);
          }
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          break;
        case DISPLAY_NEXT:
          if (config.getLocked()) {
            break;
          }
          if (list_directory(current_file.parent_path()).size() <= 1) {
            break;
          }
          in.reset();
          try {
            current_file = files_get_next(current_file);
            in.reset(display_file(current_file.c_str(), pGIFBuf, board->getDisplay()));
          } catch (std::out_of_range &err) {
            display_no_image(board->getDisplay(), pGIFBuf);
          }
          break;
        case DISPLAY_PREVIOUS:
          if (config.getLocked()) {
            break;
          }
          if (list_directory(current_file.parent_path()).size() <= 1) {
            break;
          }
          in.reset();
          try {
            current_file = files_get_previous(current_file);
            in.reset(display_file(current_file.c_str(), pGIFBuf, board->getDisplay()));
          } catch (std::out_of_range &err) {
            display_no_image(board->getDisplay(), pGIFBuf);
          }
          break;
        case DISPLAY_BATT:
          in.reset();
          if (last_mode != DISPLAY_BATT) {
            display_image_batt(board->getDisplay(), pGIFBuf);
          }
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          break;
        case DISPLAY_OTA:
          in.reset();
          display_ota(board->getDisplay(), pGIFBuf, 0);
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          break;
        case DISPLAY_NO_STORAGE:
          in.reset();
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          display_no_storage(board->getDisplay(), pGIFBuf);
          break;
        case DISPLAY_SPECIAL_1:
          ESP_LOGI(TAG, "DISPLAY_SPECIAL_1");
          config.reload();
          if(exists(std::filesystem::path("/data/cards/up.png"))) {
            in.reset();
            in.reset(display_file("/data/cards/up.png", pGIFBuf, board->getDisplay()));
            last_mode = static_cast<DISPLAY_OPTIONS>(option);
          }
          break;
        case DISPLAY_SPECIAL_2:
          ESP_LOGI(TAG, "DISPLAY_SPECIAL_2");
          config.reload();
          if(exists(std::filesystem::path("/data/cards/down.png"))) {
            in.reset();
            in.reset(display_file("/data/cards/down.png", pGIFBuf, board->getDisplay()));
            last_mode = static_cast<DISPLAY_OPTIONS>(option);
          };
          break;
        default:
          break;
      }
    } else {
      if (!lvgl_menu_state()) {
        if (last_mode == DISPLAY_FILE && config.getSlideShow()
            && ((esp_timer_get_time() / 1000000) - (last_change / 1000000)) > config.getSlideShowTime()) {
          xTaskNotifyIndexed(xTaskGetCurrentTaskHandle(), 0, DISPLAY_NEXT, eSetValueWithOverwrite);
        } else if (last_mode == DISPLAY_OTA) {
          int percent = OTA::ota_status();
          if (percent != 0) {
            display_ota(board->getDisplay(), pGIFBuf, percent);
          }
          delay = 1000;
        } else {
          if(oldMenuState){
            in.reset(display_file(current_file.c_str(), pGIFBuf, board->getDisplay()));
          }
          delay = image_loop(in, pGIFBuf, board->getDisplay());
          if (delay < 0) {
            std::string strerr = "Error Displaying File\n";
            strerr += current_file.string() + "\n" + in->getLastError();
            display_err(board->getDisplay(), pGIFBuf, strerr.c_str());
            in.reset();
          }
        }
      } else {
        delay = 200;
      }
      oldMenuState = lvgl_menu_state();
    }
  }
}