#include "FreeRTOS.h"

#include "portable_time.h"
#include "log.h"

#include "ota.h"

#include "display.h"
#include "image.h"
#include "png.h"
#include "images/low_batt_png.h"


#include "font_render.h"
#include "ui/menu.h"

#include "directory.h"
#include "file.h"
#include "dirname.h"
#include "hw_init.h"

static const char *TAG = "DISPLAY";

static DIR_SORTED dir;

static int file_position;

#define MAX_FILE_LEN 128

static void clear_screen(Display *display) {
  for(int x = 0; x <=1; x++) {
    memset(display->buffer, 255, display->size.first * display->size.second * 2);
    display->write(0, 0, display->size.first, display->size.second, display->buffer);
  }
}

static void display_no_image(Display *display) {
  LOGI(TAG, "Displaying No Image");
  clear_screen(display);
  render_text_centered(display->size.first, display->size.second, 10, "No Image", display->buffer);
  display->write(0, 0, display->size.first, display->size.second, display->buffer);
}

static void display_image_too_large(Display *display, const char *path) {
  LOGI(TAG, "Displaying Image To Large");
  clear_screen(display);
  char tmp[255];
  sprintf(tmp, "Image too Large\n%s", path);
  render_text_centered(display->size.first, display->size.second, 10, tmp, display->buffer);
  display->write(0, 0, display->size.first, display->size.second, display->buffer);
}

static void display_err(Display *display, const char *err) {
  LOGI(TAG, "Displaying Error");
  clear_screen(display);
  render_text_centered(display->size.first, display->size.second, 10, err, display->buffer);
  display->write(0, 0, display->size.first, display->size.second, display->buffer);
}

static void display_ota(Display *display, uint32_t percent) {
  LOGI(TAG, "Displaying OTA Status");
  clear_screen(display);
  char tmp[50];
  sprintf(tmp, "Update In Progress\n%lu%%", percent);
  render_text_centered(display->size.first, display->size.second, 10, tmp, display->buffer);
  display->write(0, 0, display->size.first, display->size.second, display->buffer);
}

static void display_no_storage(Display *display) {
  LOGI(TAG, "Displaying No Storage");
  clear_screen(display);
  render_text_centered(display->size.first, display->size.second, 10, "No SDCARD", display->buffer);
  display->write(0, 0, display->size.first, display->size.second, display->buffer);
}

static void display_image_batt(Display *display) {
  LOGI(TAG, "Displaying Low Battery");
  clear_screen(display);
  PNGImage png;
  png.open((uint8_t *)low_batt_png, sizeof(low_batt_png));
  int16_t xOffset = static_cast<int16_t>((display->size.first / 2) - (png.size().first / 2));
  int16_t yOffset = static_cast<int16_t>((display->size.second / 2) - ((png.size().second + 1) / 2));
  png.loop(display->buffer, xOffset, yOffset, display->size.first);
  display->write(0, 0, display->size.first, display->size.second, display->buffer);
}

static std::pair<int16_t, int16_t> lastSize = {0,0};

//#define FRAMETIME

static int displayFile(std::unique_ptr<Image> &in, Display *display) {
  int64_t start = millis();
  int delay;
  int16_t xOffset = 0;
  int16_t yOffset = 0;
  if (in->size() != display->size) {
    if(lastSize > in->size()) {
      clear_screen(display); //Only need to clear the screen if the image won't fill it, and the last image was bigger
    }
    xOffset = static_cast<int16_t>((display->size.first / 2) - (in->size().first / 2));
    yOffset = static_cast<int16_t>((display->size.second / 2) - ((in->size().second + 1) / 2));
  }
  delay = in->loop(display->buffer, xOffset, yOffset, display->size.first);
  if (delay < 0) {
    LOGI(TAG, "Image loop error");
    return -1;
  } else {
    display->write(0, 0, display->size.first, display->size.second, display->buffer);
  }
  int calc_delay = delay - static_cast<int>(millis() - start);
#ifdef FRAMETIME
  LOGI(TAG, "Frame Delay: %i, calculated delay %i", delay, calc_delay);
#endif
  lastSize = in->size();
  if(in->animated()) {
    return calc_delay > 0 ? calc_delay : 0;
  }
  else{
    return 60*1000;
  }
}

static int validator(const char *path, const char *file) {
  char inPath[128];
  JOIN_PATH(inPath, path, file);
  return valid_image_file(inPath, extensions);
}

static int get_file(char *path) {
  //Check if we are starting with a valid file, and just return it if we are
  if (valid_image_file(path, extensions)) {
    if(!dir.dirptr){
      char *base = basename(path);
      base[-1] = '\0'; //Replace the slash with a null, so we can pretend the string is shorter
      opendir_sorted(&dir, path, validator);
      base[-1] = '/'; //return the slash, fix the string
    }
    file_position = directory_get_position(&dir, basename(path));
    LOGI(TAG, "%i", file_position);
    return 0;
  }
  char *base = path;
  //Check if it's a directory
  if (!is_directory(path)) {
    //It's not a directory. Get the directory
    base = dirname(path);
  }
  struct dirent *de;  // Pointer for directory entry

  closedir_sorted(&dir);

  if (!opendir_sorted(&dir, base, validator)) {
    return -1;
  }

  while ((de = readdir_sorted(&dir)) != nullptr) {
    size_t len = strlen(path);
    if(path[len-1] != '/'){
      path[len] = '/';
      path[len+1] = '\0';
    }
    LOGI(TAG, "%s", path);
    strcpy(&path[strlen(path)], de->d_name);
    file_position = directory_get_position(&dir, de->d_name);
    LOGI(TAG, "%i", file_position);
    LOGI(TAG, "%s", path);
    return 0;
  }
  path[0] = '\0';
  return -1;
}

static Image *openFile(const char *path, Display *display) {
  Image *in = ImageFactory(path);
  if (in) {
    if (in->open(path, get_board()->turboBuffer()) != 0) {
      char errorString[255];
      snprintf(errorString, sizeof(errorString), "Error Displaying File\n%s\n%s", path, in->getLastError());
      display_err(display, errorString);
      delete in;
      return nullptr;
    }
    printf("%s x: %i y: %i\n", path, in->size().first, in->size().second);
    auto size = in->size();
    if (size > display->size) {
      delete in;
      display_image_too_large(display, path);
      return nullptr;
    }
  } else {
    char errMsg[255];
    snprintf(errMsg, sizeof(errMsg), "Could not Display\n%s", path);
    display_err(display, errMsg);
  }
  return in;
}

static Image *openFileUpdatePath(char *path, Display *display) {
  if (get_file(path) != 0) {
    display_no_image(display);
    return nullptr;
  }
  return openFile(path, display);
}

static int64_t last_change;

static bool slideshowChange(DISPLAY_OPTIONS last_mode, Config *config){
  int64_t lastChange = millis() - last_change;
  return last_mode == DISPLAY_FILE && config->getSlideShow() &&  lastChange > config->getSlideShowTime();
}

static void next_prev(std::unique_ptr<Image> &in, char *current_file, Config *config, Display *display, int increment){
  if (config->getLocked()) {
    return;
  }
  strcpy(basename(current_file), directory_get_increment(&dir, file_position, increment));
  in.reset(openFileUpdatePath(current_file, display));
}

void display_task(void *params) {
  auto *board = (Board *) params;

  auto config = board->getConfig();

  auto display = board->getDisplay();

  memset(display->buffer, 255, display->size.first * display->size.second * 2);
  display->write(0, 0, display->size.first, display->size.second, display->buffer);

  board->getBacklight()->setLevel(board->getConfig()->getBacklight() * 10);

  std::unique_ptr<Image> in = nullptr;

  char current_file[MAX_FILE_LEN+1];

  DISPLAY_OPTIONS last_mode = DISPLAY_NONE;

  last_change = millis();

  LOGI(TAG, "Display Resolution %ix%i", display->size.first, display->size.second);

  int delay = 1000;
//  vTaskDelay(1000/portTICK_PERIOD_MS);
  bool redraw = false;
  while (true) {
    uint32_t option;
    xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, delay / portTICK_PERIOD_MS);
    if (option != DISPLAY_NONE) {
      last_change = millis();
      config->reload();
      if (!(option & noResetBit)) {
        in.reset();
      }
      switch (option) {
        case DISPLAY_FILE:
          LOGI(TAG, "DISPLAY_FILE");
          if (!valid_image_file(current_file, extensions)) {
            config->getPath(current_file);
          }
          in.reset(openFileUpdatePath(current_file, display));
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          break;
        case DISPLAY_NEXT:
          next_prev(in, current_file, config, display, 1);
          break;
        case DISPLAY_PREVIOUS:
          next_prev(in, current_file, config, display, -1);
          break;
        case DISPLAY_BATT:
          if (last_mode != DISPLAY_BATT) {
            display_image_batt(board->getDisplay());
          }
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          break;
        case DISPLAY_OTA:
          display_ota(board->getDisplay(), 0);
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          break;
        case DISPLAY_NO_STORAGE:
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          display_no_storage(board->getDisplay());
          break;
        case DISPLAY_SPECIAL_1:
          LOGI(TAG, "DISPLAY_SPECIAL_1");
          if (is_file("/data/cards/up.png")) {
            in.reset(openFile("/data/cards/up.png", display));
            last_mode = static_cast<DISPLAY_OPTIONS>(option);
          }
          break;
        case DISPLAY_SPECIAL_2:
          LOGI(TAG, "DISPLAY_SPECIAL_2");
          if (is_file("/data/cards/down.png")) {
            in.reset(openFile("/data/cards/down.png", display));
            last_mode = static_cast<DISPLAY_OPTIONS>(option);
          }
          break;
        case DISPLAY_NOTIFY_CHANGE:
          redraw = true;
          break;
        case DISPLAY_NOTIFY_USB:
          in.reset();
          closedir_sorted(&dir);
          dir.dirptr = nullptr;
        default:
          break;
      }
    }
    delay = 1000;
    if (!lvgl_menu_state()) {
      if (slideshowChange(last_mode, config)) {
        xTaskNotifyIndexed(xTaskGetCurrentTaskHandle(), 0, DISPLAY_NEXT, eSetValueWithOverwrite);
      } else if (last_mode == DISPLAY_OTA) {
#ifdef ESP_PLATFORM
        int percent = OTA::ota_status();
        if (percent != 0) {
          display_ota(display, percent);
        }
        delay = 1000;
#endif
      } else {
        if(redraw){
          config->getPath(current_file);
          in.reset(openFileUpdatePath(current_file, display));
          redraw = false;
        }
        if(in) {
          delay = displayFile(in, display);
          if (delay < 0) {
            char errMsg[255];
            snprintf(errMsg, sizeof(errMsg), "Error Displaying File\n%s\n%s",  current_file, in->getLastError());
            display_err(board->getDisplay(), errMsg);
            in.reset();
          }
        }
      }
    }
  }
}