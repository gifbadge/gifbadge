#include <esp_log.h>
#include <cstring>
#include "file_util.h"
#include "image.h"

static const char *TAG = "FILE_UTIL";

/*!
 * Checks if the file is not a directory, and not hidden
 * Only works on files in /data
 * @param path
 * @return
 */
bool valid_file(const char *path){
  ESP_LOGI(TAG, "%s", path);

  const char *ext = strrchr(path, '.');
  bool matched = false;
  if(ext != nullptr) {
    for (auto & extension : extensions) {
      if (strcasecmp(extension, ext) == 0) {
        matched = true;
        break;
      }
    }
  }
  if(!matched){
    return false;
  }
  //strip the leading /data/ for fat fs

  FILINFO info;
  if (f_stat(&path[5], &info) == 0) {
    ESP_LOGI(TAG, "%s %i %i", path, info.fattrib & AM_DIR, info.fattrib & AM_HID);
    if (!((info.fattrib & AM_DIR) || (info.fattrib & AM_HID))) {
      if (basename(path)[0] != '.' && basename(path)[0] != '~') {
        return true;
      }
    }
  }
  return false;
}