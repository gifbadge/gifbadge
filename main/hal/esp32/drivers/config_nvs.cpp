#include <nvs_flash.h>
#include "log.h"
#include "hal/esp32/drivers/config_nvs.h"

Config_NVS::Config_NVS() {
  esp_err_t err;
  err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  handle = nvs::open_nvs_handle("image", NVS_READWRITE, &err);
}

template<typename T>
T Config_NVS::get_item_or_default(const char *item, T value) {
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

void Config_NVS::get_string_or_default(const char *item, const char *default_value, char *out, size_t out_len) {
  if (handle->get_string(item, out, out_len) == ESP_ERR_NVS_NOT_FOUND) {
    handle->set_string(item, default_value);
    handle->commit();
  }
}

void Config_NVS::setPath(const char *path) {
  handle->set_string("path", path);
}

void Config_NVS::getPath(char *outPath) {
  get_string_or_default("path", (const char *) "/data", outPath, 128);
}

void Config_NVS::setLocked(bool locked) {
  handle->set_item("locked", locked);
}

bool Config_NVS::getLocked() {
  return get_item_or_default("locked", false);
}

bool Config_NVS::getSlideShow() {
  return get_item_or_default("slideshow", false);
}

void Config_NVS::setSlideShow(bool slideshow) {
  handle->set_item("slideshow", slideshow);
}

int Config_NVS::getSlideShowTime() {
  return get_item_or_default("slideshow_time", 15);
}

void Config_NVS::setSlideShowTime(int slideshow_time) {
  handle->set_item("slideshow_time", slideshow_time);
}

int Config_NVS::getBacklight(){
  return get_item_or_default("backlight", 10);
}

void Config_NVS::setBacklight(int backlight) {
  handle->set_item("backlight", backlight);
}


void Config_NVS::reload() {

}
void Config_NVS::save() {

}
