#pragma once

#include <string>
#include <mutex>

#include <nvs_flash.h>
#include <nvs_handle.hpp>




class ImageConfig {
public:
    ImageConfig();
    ~ImageConfig();
    void save();

    void setDirectory(const std::string&);
    std::string getDirectory();
    void setFile(const std::string& );
    std::string getFile();
    void setLocked(bool);
    bool getLocked();
    bool getSlideShow();
    void setSlideShow(bool);
    int getSlideShowTime();
    void setSlideShowTime(int);


private:
    std::unique_ptr<nvs::NVSHandle> handle;
    template<typename T> T get_item_or_default(const char *key, T value);
    std::string get_string_or_default(const char *item, std::string value);

    std::mutex mutex;
    std::string directory;
    std::string image_file;
    bool locked;
    bool slideshow;
    int slideshow_time;
};

class BatteryConfig {
public:
    void setVoltage(double v);
    double getVoltage();

private:
    std::mutex mutex;
    double voltage;
};