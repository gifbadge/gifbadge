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

static void clear_screen(const std::shared_ptr<Display> &display, uint8_t *pBuf) {
  if (pBuf != nullptr) {
    memset(pBuf, 255, display->size.first * display->size.second * 2);
    display->write(0, 0, display->size.first, display->size.second, pBuf);
  }
}

static void display_no_image(const std::shared_ptr<Display> &display, uint8_t *pGIFBuf) {
  ESP_LOGI(TAG, "Displaying No Image");
  clear_screen(display, pGIFBuf);
  render_text_centered(display->size.first, display->size.second, 10, "No Image", pGIFBuf);
  display->write(0, 0, display->size.first, display->size.second, pGIFBuf);
}

static void display_image_too_large(const std::shared_ptr<Display> &display, uint8_t *pGIFBuf, const char *path) {
  ESP_LOGI(TAG, "Displaying Image To Large");
  clear_screen(display, pGIFBuf);
  char tmp[255];
  sprintf(tmp, "Image too Large\n%s", path);
  render_text_centered(display->size.first, display->size.second, 10, tmp, pGIFBuf);
  display->write(0, 0, display->size.first, display->size.second, pGIFBuf);
}

static void display_err(const std::shared_ptr<Display> &display, uint8_t *pGIFBuf, const char *err) {
  ESP_LOGI(TAG, "Displaying Error");
  clear_screen(display, pGIFBuf);
  render_text_centered(display->size.first, display->size.second, 10, err, pGIFBuf);
  display->write(0, 0, display->size.first, display->size.second, pGIFBuf);
}

static void display_ota(const std::shared_ptr<Display> &display, uint8_t *pGIFBuf, uint32_t percent) {
  ESP_LOGI(TAG, "Displaying OTA Status");
  clear_screen(display, pGIFBuf);
  char tmp[50];
  sprintf(tmp, "Update In Progress\n%lu%%", percent);
  render_text_centered(display->size.first, display->size.second, 10, tmp, pGIFBuf);
  display->write(0, 0, display->size.first, display->size.second, pGIFBuf);
}

static void display_no_storage(const std::shared_ptr<Display> &display, uint8_t *pGIFBuf) {
  ESP_LOGI(TAG, "Displaying No Storage");
  clear_screen(display, pGIFBuf);
  render_text_centered(display->size.first, display->size.second, 10, "No SDCARD", pGIFBuf);
  display->write(0, 0, display->size.first, display->size.second, pGIFBuf);
}

static void display_image_batt(const std::shared_ptr<Display> &display, uint8_t *buf) {
  ESP_LOGI(TAG, "Displaying Low Battery");
  clear_screen(display, buf);
  PNGImage png;
  png.open(low_batt_png, sizeof(low_batt_png));
  int16_t xOffset = (display->size.first / 2) - (png.size().first / 2);
  int16_t yOffset = (display->size.second / 2) - ((png.size().second + 1) / 2);
  png.loop(buf, xOffset, yOffset, display->size.first);
  display->write(0, 0, display->size.first, display->size.second, buf);
}

static std::pair<int16_t, int16_t> lastSize = {0,0};

//#define FRAMETIME

static int displayFile(Image *in, uint8_t *pGIFBuf, const std::shared_ptr<Display> &display) {
  int64_t start = esp_timer_get_time();
  int delay;
  int16_t xOffset = 0;
  int16_t yOffset = 0;
  if (in->size() != display->size) {
    if(lastSize > in->size()) {
      clear_screen(display, pGIFBuf); //Only need to clear the screen if the image won't fill it, and the last image was bigger
    }
    xOffset = (display->size.first / 2) - (in->size().first / 2);
    yOffset = (display->size.second / 2) - ((in->size().second + 1) / 2);
  }
  ESP_LOGI(TAG, "%d %d", xOffset, yOffset);
  delay = in->loop(pGIFBuf, xOffset, yOffset, display->size.first);
  if (delay < 0) {
    ESP_LOGI(TAG, "Image loop error");
    return -1;
  } else {
    display->write(0, 0, display->size.first, display->size.second, pGIFBuf);
  }
  int calc_delay = delay - static_cast<int>((esp_timer_get_time() - start) / 1000);
#ifdef FRAMETIME
  ESP_LOGI(TAG, "Frame Delay: %i, calculated delay %i", delay, calc_delay);
#endif
  lastSize = in->size();
  if(in->animated()) {
    return calc_delay > 0 ? calc_delay : 0;
  }
  else{
    return 60*1000;
  }
}

static int get_file(const char *path, char *outPath, size_t outLen) {
  char inPath[128];
  strncpy(inPath, path, sizeof(inPath)-1);

  //Check if we are starting with a valid file, and just return it if we are
  if (valid_file(path)) {
    strncpy(outPath, inPath, outLen - 1);
    return 0;
  }
  char *base = inPath;
  //Check if it's a directory
  if (!is_dir(path)) {
    //It's not a directory. Get the directory
    base = dirname(inPath);
  }
  struct dirent *de;  // Pointer for directory entry

  DIR *dr = opendir(base);

  if (dr == nullptr) {
    return -1;
  }

  while ((de = readdir(dr)) != nullptr) {
    if (snprintf(outPath, outLen, "%s/%s", base, de->d_name) < 0) {
      return -1;
    }
    if (valid_file(outPath)) {
      ESP_LOGI(TAG, "%s", outPath);
      closedir(dr);
      return 0;
    }
  }
  closedir(dr);
  outPath[0] = '\0';
  return -1;
}

Image *openFile(const char *path, uint8_t *pGIFBuf, const std::shared_ptr<Display> &display) {
  Image *in = ImageFactory(path);
  if (in) {
    if (in->open(path) != 0) {
      char errorString[255];
      snprintf(errorString, sizeof(errorString), "Error Displaying File\n%s\n%s", path, in->getLastError());
      display_err(display, pGIFBuf, errorString);
      delete in;
      return nullptr;
    }
    printf("%s x: %i y: %i\n", path, in->size().first, in->size().second);
    auto size = in->size();
    if (size > display->size) {
      delete in;
      display_image_too_large(display, pGIFBuf, path);
      return nullptr;
    }
  } else {
    char errMsg[255];
    snprintf(errMsg, sizeof(errMsg), "Could not Display\n%s", path);
    display_err(display, pGIFBuf, errMsg);
  }
  return in;
}

Image *openFileUpdatePath(char *path, size_t pathLen, uint8_t *pGIFBuf, const std::shared_ptr<Display> &display){
  if (get_file(path, path, pathLen) != 0) {
    display_no_image(display, pGIFBuf);
    return nullptr;
  }
  return openFile(path, pGIFBuf, display);
}



static int files_get_position(const std::vector<std::string> &files, const std::string &name) {
//    auto files = list_directory(path);
  auto result = std::find(files.begin(), files.end(), name.c_str());
  if (result == files.end() ||files.empty()) {
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
    return "";
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
    return "";
  }
}

static int64_t last_change;

static bool slideshowChange(DISPLAY_OPTIONS last_mode, ImageConfig &config){
  int64_t lastChange = ((esp_timer_get_time() / 1000000) - (last_change / 1000000));
  return last_mode == DISPLAY_FILE && config.getSlideShow() &&  lastChange > config.getSlideShowTime();
}

void display_task(void *params) {
  // TODO: Check image size before trying to display it
  auto *board = (Board *) params;

  auto config = ImageConfig();

  // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
  uint8_t *pGIFBuf = board->getDisplay()->getBuffer();

  memset(pGIFBuf, 255, board->getDisplay()->size.first * board->getDisplay()->size.second * 2);
  board->getDisplay()->write(0, 0, board->getDisplay()->size.first, board->getDisplay()->size.second, pGIFBuf);

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

  char current_file[128];

  DISPLAY_OPTIONS last_mode = DISPLAY_NONE;

  last_change = esp_timer_get_time();

  ESP_LOGI(TAG, "Display Resolution %ix%i", board->getDisplay()->size.first, board->getDisplay()->size.second);

  int delay = 1000;
  while (true) {
    uint32_t option;
    pGIFBuf = board->getDisplay()->buffer;
    xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, delay / portTICK_PERIOD_MS);
    if (option != DISPLAY_NONE) {
      last_change = esp_timer_get_time();
      config.reload();
      in.reset();
      switch (option) {
        case DISPLAY_FILE:
          ESP_LOGI(TAG, "DISPLAY_FILE");
          strncpy(current_file, config.getPath().c_str(), sizeof(current_file)-1);
          in.reset(openFileUpdatePath(current_file, sizeof(current_file), pGIFBuf, board->getDisplay()));
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          break;
        case DISPLAY_NEXT:
          if (config.getLocked()) {
            break;
          }
          if(!files_get_next(current_file).empty()){
            strncpy(current_file,  files_get_next(current_file).c_str(), sizeof(current_file)-1);
            in.reset(openFileUpdatePath(current_file, sizeof(current_file), pGIFBuf, board->getDisplay()));
          }
          break;
        case DISPLAY_PREVIOUS:
          if (config.getLocked()) {
            break;
          }
          if(!files_get_previous(current_file).empty()){
            strncpy(current_file, files_get_previous(current_file).c_str(), sizeof(current_file)-1);
            in.reset(openFileUpdatePath(current_file, sizeof(current_file), pGIFBuf, board->getDisplay()));
          }
          break;
        case DISPLAY_BATT:
          if (last_mode != DISPLAY_BATT) {
            display_image_batt(board->getDisplay(), pGIFBuf);
          }
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          break;
        case DISPLAY_OTA:
          display_ota(board->getDisplay(), pGIFBuf, 0);
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          break;
        case DISPLAY_NO_STORAGE:
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          display_no_storage(board->getDisplay(), pGIFBuf);
          break;
        case DISPLAY_SPECIAL_1:
          ESP_LOGI(TAG, "DISPLAY_SPECIAL_1");
          if (valid_file("/data/cards/up.png")) {
            in.reset(openFile("/data/cards/up.png", pGIFBuf, board->getDisplay()));
            last_mode = static_cast<DISPLAY_OPTIONS>(option);
          }
          break;
        case DISPLAY_SPECIAL_2:
          ESP_LOGI(TAG, "DISPLAY_SPECIAL_2");
          if (valid_file("/data/cards/down.png")) {
            in.reset(openFile("/data/cards/down.png", pGIFBuf, board->getDisplay()));
            last_mode = static_cast<DISPLAY_OPTIONS>(option);
          }
          break;
        default:
          break;
      }
    }
    delay = 1000;
    if (!lvgl_menu_state()) {
      if (slideshowChange(last_mode, config)) {
        xTaskNotifyIndexed(xTaskGetCurrentTaskHandle(), 0, DISPLAY_NEXT, eSetValueWithOverwrite);
      } else if (last_mode == DISPLAY_OTA) {
        int percent = OTA::ota_status();
        if (percent != 0) {
          display_ota(board->getDisplay(), pGIFBuf, percent);
        }
        delay = 1000;
      } else {
        if(in) {
          delay = displayFile(in.get(), pGIFBuf, board->getDisplay());
          if (delay < 0) {
            char errMsg[255];
            snprintf(errMsg, sizeof(errMsg), "Error Displaying File\n%s\n%s",  current_file, in->getLastError());
            display_err(board->getDisplay(), pGIFBuf, errMsg);
            in.reset();
          }
        }
      }
    }
  }
}