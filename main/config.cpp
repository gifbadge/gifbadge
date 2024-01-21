#include "config.h"
#include <nvs_flash.h>
#include <nvs_handle.hpp>

static const char *TAG = "CONFIG";


ImageConfig::ImageConfig() {
    esp_err_t err;
    handle = nvs::open_nvs_handle("image", NVS_READWRITE, &err);
    directory = get_string_or_default("directory", (const char *)"/data");
    image_file = get_string_or_default("image_file", (const char *)"");
    locked = get_item_or_default("locked", false);
    slideshow = get_item_or_default("slideshow", false);
    slideshow_time = get_item_or_default("slideshow_time", 15);
}

ImageConfig::~ImageConfig(){
    handle->set_string("directory", directory.c_str());
    handle->set_string("image_file", image_file.c_str());
    handle->set_item("locked", locked);
    handle->set_item("slideshow", slideshow);
    handle->set_item("slideshow_time", slideshow_time);
    handle->commit();
}

void ImageConfig::save() {
    handle->set_string("directory", directory.c_str());
    handle->set_string("image_file", image_file.c_str());
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

std::string ImageConfig::get_string_or_default(const char *item, std::string value) {
    esp_err_t err;
    char ret[128];
    err = handle->get_string(item, ret, 128);
    switch (err) {
        case ESP_OK:
            return ret;
        case ESP_ERR_NVS_NOT_FOUND:
            handle->set_string(item, value.c_str());
            handle->commit();
            return value;
        default :
            return value;
    }
}

void ImageConfig::setDirectory(const std::filesystem::path &value) {
    const std::lock_guard<std::mutex> lock(mutex);
    directory = value;
}

std::filesystem::path ImageConfig::getDirectory() {
    const std::lock_guard<std::mutex> lock(mutex);
    return directory;
}

void ImageConfig::setFile(const std::filesystem::path &value) {
    const std::lock_guard<std::mutex> lock(mutex);
    image_file = value;
}

std::filesystem::path ImageConfig::getFile() {
    const std::lock_guard<std::mutex> lock(mutex);
    return image_file;
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