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