#include "FreeRTOS.h"

#include "portable_time.h"
#include "log.h"

#include "ota.h"

#include "display.h"
#include <memory>
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

static void clear_screen(Display *display, bool flush = true) {
  for (int x = 0; x <= 1; x++) {
    memset(display->buffer, 255, display->size.first * display->size.second * 2);
    if (flush) {
      display->write(0, 0, display->size.first, display->size.second, display->buffer);
    }
  }
}

class ErrorImage : public Image {
 public:
  ErrorImage(std::pair<int16_t, int16_t> size, const char *error)
      : _width(size.first), _height(size.second), _error("") {
    if (error != nullptr) {
      strcpy(_error, error);
    }
  }

  template<typename ... Args>
  ErrorImage(std::pair<int16_t, int16_t> size, const char *fmt, Args &&... args)
      : _width(size.first), _height(size.second), _error("") {
    snprintf(_error, sizeof(_error) - 1, fmt, std::forward<Args>(args) ...);
  };

  frameReturn loop(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) override {
    memset(outBuf, 255, _width * _height * 2);
    render_text_centered(_width, _height, 10, _error, outBuf);
    return {frameStatus::OK, _delay};
  }
  std::pair<int16_t, int16_t> size() override {
    return {_width, _height};
  }
  const char *getLastError() override {
    return nullptr;
  }
 protected:
  int16_t _width;
  int16_t _height;
  char _error[255];
  int _delay = 1000;
};

class NoImage : public ErrorImage {
 public:
  explicit NoImage(std::pair<int16_t, int16_t> size) : ErrorImage(size, nullptr) {
    strcpy(_error, "No Image");
  }
};

class TooLargeImage : public ErrorImage {
 public:
  TooLargeImage(std::pair<int16_t, int16_t> size, const char *path) : ErrorImage(size, nullptr) {
    sprintf(_error, "Image too Large\n%s", path);
  }
};

class OTAImage : public ErrorImage {
 public:
  explicit OTAImage(std::pair<int16_t, int16_t> size) : ErrorImage(size, nullptr) {
    strcpy(_error, "Update In Progress\n");
    _delay = 500;
  }
  frameReturn loop(uint8_t *outBuf, int16_t x, int16_t y, int16_t width) override {
#ifdef ESP_PLATFORM
    int percent = OTA::ota_status();
    sprintf(_error, "Update In Progress\n%d%%", percent);
#endif
    return ErrorImage::loop(outBuf, x, y, width);
  }
  bool animated() override {
    return true;
  }
};

class NoStorageImage : public ErrorImage {
 public:
  explicit NoStorageImage(std::pair<int16_t, int16_t> size) : ErrorImage(size, nullptr) {
    strcpy(_error, "No SDCARD");
  }
};

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

static frameReturn displayFile(std::unique_ptr<Image> &in, Display *display) {
  int64_t start = millis();
  frameReturn status;
  int16_t xOffset = 0;
  int16_t yOffset = 0;
  if (in->size() != display->size) {
    if(lastSize > in->size()) {
      clear_screen(display,
                   false); //Only need to clear the screen if the image won't fill it, and the last image was bigger
    }
    xOffset = static_cast<int16_t>((display->size.first / 2) - (in->size().first / 2));
    yOffset = static_cast<int16_t>((display->size.second / 2) - ((in->size().second + 1) / 2));
  }
  status = in->loop(display->buffer, xOffset, yOffset, display->size.first);
  if (status.first == frameStatus::ERROR) {
    LOGI(TAG, "Image loop error");
    return {frameStatus::ERROR, 0};
  } else {
    display->write(0, 0, display->size.first, display->size.second, display->buffer);
  }
  int calc_delay = status.second - static_cast<int>(millis() - start);
#ifdef FRAMETIME
  LOGI(TAG, "Frame Delay: %i, calculated delay %i", delay, calc_delay);
#endif
  lastSize = in->size();
  if(in->animated()) {
    return {status.first, calc_delay > 0 ? calc_delay : 0};
  }
  else{
    return {frameStatus::END, 15 * 1000};
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
//  path[0] = '\0';
  return -1;
}

static Image *openFile(const char *path, Display *display) {
  Image *in = ImageFactory(path);
  if (in) {
    if (in->open(path, get_board()->turboBuffer()) != 0) {
      const char *lastError = in->getLastError();
      delete in;
      return new ErrorImage(display->size, "Error Displaying File\n%s\n%s", path, lastError);
    }
    printf("%s x: %i y: %i\n", path, in->size().first, in->size().second);
    auto size = in->size();
    if (size > display->size) {
      delete in;
      return new TooLargeImage(display->size, path);
    }
  } else {
    return new ErrorImage(display->size, "Could not Display\n%s", path);
  }
  return in;
}

static Image *openFileUpdatePath(char *path, Display *display) {
  if (get_file(path) != 0) {
    file_position = -1;
    return new NoImage(display->size);
  }
  return openFile(path, display);
}


static void next_prev(std::unique_ptr<Image> &in, char *current_file, Config *config, Display *display, int increment){
  if (config->getLocked()) {
    return;
  }
  const char *next = directory_get_increment(&dir, file_position, increment);
  if (next != nullptr) {
    strcpy(basename(current_file), next);
    in.reset(); //Free the memory before trying to open the next image
    in.reset(openFileUpdatePath(current_file, display));
  }
}

TimerHandle_t slideShowTimer = nullptr;

static void slideShowHandler(TimerHandle_t) {
  TaskHandle_t displayHandle = xTaskGetHandle("display_task");
  xTaskNotifyIndexed(displayHandle, 0, DISPLAY_ADVANCE, eSetValueWithOverwrite);
}

static void slideShowStart(Config *config) {
  if (config->getSlideShow()) {
    xTimerChangePeriod(slideShowTimer, (config->getSlideShowTime() * 1000) / portTICK_PERIOD_MS, 0);
    xTimerStart(slideShowTimer, 0);
  } else {
    xTimerStop(slideShowTimer, 0);
  }
}

void display_task(void *params) {
  auto *board = (Board *) params;

  auto config = board->getConfig();

  auto display = board->getDisplay();

  slideShowTimer = xTimerCreate("slideshow", 100 / portTICK_PERIOD_MS, pdTRUE, nullptr, slideShowHandler);

  memset(display->buffer, 255, display->size.first * display->size.second * 2);
  display->write(0, 0, display->size.first, display->size.second, display->buffer);

  board->getBacklight()->setLevel(board->getConfig()->getBacklight() * 10);

  DISPLAY_OPTIONS last_mode = DISPLAY_NONE;

  LOGI(TAG, "Display Resolution %ix%i", display->size.first, display->size.second);

  int delay = 1000; //Delay for display loop. Is adjusted by the results of the loop method of the image being displayed
  bool redraw = false; //Reload from configuration next time we go to display an image
  bool advance = false; //Advance the slideshow
  bool endOfFrame = false; //Last frame of animation
  std::unique_ptr<Image> in = nullptr; //The image we are displaying
  char current_file[MAX_FILE_LEN + 1]; //The current file path that has been selected
  config->getPath(current_file);
  while (true) {
    uint32_t option;
    xTaskNotifyWaitIndexed(0, 0, 0xffffffff, &option, delay / portTICK_PERIOD_MS);
    if (option != DISPLAY_NONE) {
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
          slideShowStart(config);
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
          file_position = -1;
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          break;
        case DISPLAY_OTA:
          in = std::make_unique<OTAImage>(display->size);
          file_position = -1;
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          break;
        case DISPLAY_NO_STORAGE:
          last_mode = static_cast<DISPLAY_OPTIONS>(option);
          in = std::make_unique<NoStorageImage>(display->size);
          file_position = -1;
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
          closedir_sorted(&dir);
          dir.dirptr = nullptr;
          break;
        case DISPLAY_ADVANCE:
          advance = true;
          break;
        default:
          break;
      }
    }
    delay = 1000;

    if (lvgl_menu_state()) {
      //Go back to the top if the menu is open
      continue;
    }

    if (redraw) {
      // Something has changed in the configuration, reopen the configured file.
      config->getPath(current_file);
      in.reset(openFileUpdatePath(current_file, display));
      slideShowStart(config);
      redraw = false;
      advance = false;
      endOfFrame = false;
    }

    if (advance && endOfFrame) {
      next_prev(in, current_file, config, display, 1);
      advance = false;
      endOfFrame = false;
    }

    if (in) {
      // If there is an open file, display the next frame
      frameReturn status = displayFile(in, display);
      delay = status.second;
      if (status.first == frameStatus::ERROR) {
        if (config->getSlideShow()) {
          //Skip the error, and head to the next file in slideshow mode
          next_prev(in, current_file, config, display, 1);
          continue;
        }
        in = std::make_unique<ErrorImage>(display->size,
                                          "Error Displaying File\n%s\n%s",
                                          current_file,
                                          in->getLastError());
      } else if (status.first == frameStatus::END) {
        endOfFrame = true;
      }
    }
  }
}