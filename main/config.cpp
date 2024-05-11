#include "config.h"
#include <nvs_flash.h>
#include <nvs_handle.hpp>
#include <cstring>

static const char *TAG = "CONFIG";

ImageConfig::ImageConfig() {
  esp_err_t err;
  handle = nvs::open_nvs_handle("image", NVS_READWRITE, &err);
  get_string_or_default("path", (const char *) "/data", path, sizeof(path));
  locked = get_item_or_default("locked", false);
  slideshow = get_item_or_default("slideshow", false);
  slideshow_time = get_item_or_default("slideshow_time", 15);
}

ImageConfig::~ImageConfig() {
  handle->set_string("path", path);
  handle->set_item("locked", locked);
  handle->set_item("slideshow", slideshow);
  handle->set_item("slideshow_time", slideshow_time);
  handle->commit();
}

void ImageConfig::save() {
  handle->set_string("path", path);
  handle->set_item("locked", locked);
  handle->set_item("slideshow", slideshow);
  handle->set_item("slideshow_time", slideshow_time);
  handle->commit();
}

template<typename T>
T ImageConfig::get_item_or_default(const char *item, T value) {
  esp_err_t err;
  T ret;
  err = handle->get_item(item, ret);
  switch (err) {
    case ESP_OK:
      return ret;
    case ESP_ERR_NVS_NOT_FOUND:
      handle->set_item(item, value);
      handle->commit();
      return value;
    default :
      return value;
  }
}

void ImageConfig::get_string_or_default(const char *item, const char* default_value, char *out, size_t out_len) {
  esp_err_t err;
    if(handle->get_string(item, out, out_len) == ESP_ERR_NVS_NOT_FOUND) {
      handle->set_string(item, default_value);
      handle->commit();
    }
}

void ImageConfig::setPath(const char *value) {
  const std::lock_guard<std::mutex> lock(mutex);
  strncpy(path, value, sizeof(path)-1);
}

const char * ImageConfig::getPath() {
  const std::lock_guard<std::mutex> lock(mutex);
  return path;
}

void ImageConfig::setLocked(bool state) {
  const std::lock_guard<std::mutex> lock(mutex);
  locked = state;
}

bool ImageConfig::getLocked() {
  const std::lock_guard<std::mutex> lock(mutex);
  return locked;
}

void ImageConfig::setSlideShow(bool state) {
  const std::lock_guard<std::mutex> lock(mutex);
  slideshow = state;
}

bool ImageConfig::getSlideShow() {
  const std::lock_guard<std::mutex> lock(mutex);
  return slideshow;
}

void ImageConfig::setSlideShowTime(int t) {
  const std::lock_guard<std::mutex> lock(mutex);
  slideshow_time = t;
}

int ImageConfig::getSlideShowTime() {
  const std::lock_guard<std::mutex> lock(mutex);
  return slideshow_time;
}

void ImageConfig::reload() {
  esp_err_t err;
  handle = nvs::open_nvs_handle("image", NVS_READWRITE, &err);
  get_string_or_default("path", (const char *) "/data", path, sizeof(path));
  locked = get_item_or_default("locked", false);
  slideshow = get_item_or_default("slideshow", false);
  slideshow_time = get_item_or_default("slideshow_time", 15);
}
