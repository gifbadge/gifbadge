#include <esp_log.h>
#include <cstring>
#include <sys/stat.h>
#include "file_util.h"

static const char *TAG = "FILE_UTIL";


//From GLIBC
char *dirname(char *path) {
  static const char dot[] = ".";
  char *last_slash;
  /* Find last '/'.  */
  last_slash = path != NULL ? strrchr(path, '/') : NULL;
  if (last_slash != NULL && last_slash != path && last_slash[1] == '\0') {
    /* Determine whether all remaining characters are slashes.  */
    char *runp;
    for (runp = last_slash; runp != path; --runp)
      if (runp[-1] != '/')
        break;
    /* The '/' is the last character, we have to look further.  */
    if (runp != path)
      last_slash = static_cast<char *>(memrchr(path, '/', runp - path));
  }
  if (last_slash != NULL) {
    /* Determine whether all remaining characters are slashes.  */
    char *runp;
    for (runp = last_slash; runp != path; --runp)
      if (runp[-1] != '/')
        break;
    /* Terminate the path.  */
    if (runp == path) {
      /* The last slash is the first character in the string.  We have to
         return "/".  As a special case we have to return "//" if there
         are exactly two slashes at the beginning of the string.  See
         XBD 4.10 Path Name Resolution for more information.  */
      if (last_slash == path + 1)
        ++last_slash;
      else
        last_slash = path + 1;
    } else
      last_slash = runp;
    last_slash[0] = '\0';
  } else
    /* This assignment is ill-designed but the XPG specs require to
       return a string containing "." in any case no directory part is
       found and so a static and constant string is required.  */
    path = (char *) dot;
  return path;
}

bool is_dir(const char *path){
  struct stat buffer{};
  stat(path, &buffer);
  return (buffer.st_mode & S_IFDIR);
}

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
        return true;
      }
    }
  }
  return false;
}

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
        ESP_LOGV(TAG, "%s", current_path.c_str());
        std::string fat_path = current_path.substr(5, current_path.length());
        ESP_LOGV(TAG, "%s %s", current_path.c_str(), fat_path.c_str());
        //Check if file is hidden, a directory, or has any ignored characters.
        //This will only work on the first FatFS filesystem right now, I think?
        FILINFO info;
        if (f_stat(fat_path.c_str(), &info) == 0) {
          ESP_LOGV(TAG, "%s %i %i", current_path.c_str(), info.fattrib & AM_DIR, info.fattrib & AM_HID);
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
        ESP_LOGV(TAG, "%s", current_path.c_str());
        std::string fat_path = current_path.substr(5, current_path.length());
        ESP_LOGV(TAG, "%s %s", current_path.c_str(), fat_path.c_str());
        //Check if file is hidden or has any ignored characters.
        //This will only work on the first FatFS filesystem right now, I think?
        FILINFO info;
        if (f_stat(fat_path.c_str(), &info) == 0) {
          ESP_LOGV(TAG, "%s %i %i", current_path.c_str(), info.fattrib & AM_DIR, info.fattrib & AM_HID);
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
