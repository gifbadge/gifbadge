#include <esp_log.h>
#include <cstring>
#include <sys/stat.h>
#include "file_util.h"

static const char *TAG = "FILE_UTIL";

/*!
 * Checks if the file is not a directory, and not hidden
 * Only works on files in /data
 * @param path
 * @return
 */
bool valid_file(const char *path){
  ESP_LOGI(TAG, "%s", path);
  //strip the leading /data/ for fat fs
  FILINFO info;
  if (f_stat(&path[5], &info) == 0) {
    ESP_LOGI(TAG, "%s %i %i", path, info.fattrib & AM_DIR, info.fattrib & AM_HID);
    if (!((info.fattrib & AM_DIR) || (info.fattrib & AM_HID))) {
      if (basename(path)[0] != '.' && basename(path)[0] != '~') {
        const char *dot = strrchr(path, '.');
        if(dot != nullptr) {
          if (strcasecmp(dot, ".gif") == 0 || strcasecmp(dot, ".jpeg") == 0 || strcasecmp(dot, ".jpg") == 0
              || strcasecmp(dot, ".png") == 0) {
            //TODO: Improve this hack
            return true;
          }
        }
      }
    }
  }
  return false;
}