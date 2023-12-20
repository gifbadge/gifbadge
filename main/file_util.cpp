#include <esp_log.h>
#include "file_util.h"

static const char *TAG = "FILE_UTIL";


std::vector<std::string> list_directory(const std::string &path) {
    std::vector<std::string> files;
    DIR *dir = opendir(path.c_str());
    if (dir != nullptr) {
        while (true) {
            struct dirent *de = readdir(dir);
            if (!de) {
                break;
            }
            std::string current_path = (path + "/" + de->d_name);
            if (current_path.starts_with("/data")) {
                ESP_LOGI(TAG, "%s", current_path.c_str());
                std::string fat_path = current_path.substr(5, current_path.length());
                ESP_LOGI(TAG, "%s %s", current_path.c_str(), fat_path.c_str());
                //Check if file is hidden, a directory, or has any ignored characters.
                //This will only work on the first FatFS filesystem right now, I think?
                FILINFO info;
                if (f_stat(fat_path.c_str(), &info) == 0) {
                    ESP_LOGI(TAG, "%s %i %i", current_path.c_str(), info.fattrib & AM_DIR, info.fattrib & AM_HID);
                    if (!((info.fattrib & AM_DIR) || (info.fattrib & AM_HID))) {
                        if (de->d_name[0] != '.' && de->d_name[0] != '~') {
                            files.emplace_back(current_path);
                        }
                    }
                }
            }
        }
        closedir(dir);
    }
    return files;
}

std::vector<std::string> list_folders(const std::string &path) {
    std::vector<std::string> folders;
    DIR *dir = opendir(path.c_str());
    if (dir != nullptr) {
        while (true) {
            struct dirent *de = readdir(dir);
            if (!de) {
                break;
            }
            std::string current_path = (path + "/" + de->d_name);
            if (current_path.starts_with("/data")) {
                ESP_LOGI(TAG, "%s", current_path.c_str());
                std::string fat_path = current_path.substr(5, current_path.length());
                ESP_LOGI(TAG, "%s %s", current_path.c_str(), fat_path.c_str());
                //Check if file is hidden or has any ignored characters.
                //This will only work on the first FatFS filesystem right now, I think?
                FILINFO info;
                if (f_stat(fat_path.c_str(), &info) == 0) {
                    ESP_LOGI(TAG, "%s %i %i", current_path.c_str(), info.fattrib & AM_DIR, info.fattrib & AM_HID);
                    if ((info.fattrib & AM_DIR) && !(info.fattrib & AM_HID)) {
                        if (de->d_name[0] != '.' && de->d_name[0] != '~') {
                            folders.emplace_back(current_path);
                        }
                    }
                }
            }
        }
        closedir(dir);
    }
    return folders;
}
