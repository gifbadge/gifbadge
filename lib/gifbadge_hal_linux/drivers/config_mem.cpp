#include <cstring>
#include <csignal>
#include "drivers/config_mem.h"
void Config_Mem::setPath(const char *path) {
  strcpy(_path, path);
}
void Config_Mem::getPath(char *outPath) {
  strcpy(outPath, _path);
}
void Config_Mem::setLocked(bool) {

}
bool Config_Mem::getLocked() {
  return _locked;
}
bool Config_Mem::getSlideShow() {
  return _slideshow;
}
void Config_Mem::setSlideShow(bool slideshow) {
  _slideshow = slideshow;
}
int Config_Mem::getSlideShowTime() {
  return _slideshow_time;
}
void Config_Mem::setSlideShowTime(int time) {
  _slideshow_time = time;
}
int Config_Mem::getBacklight() {
  return _backlight;
}
void Config_Mem::setBacklight(int level) {
  _backlight = level;
}
void Config_Mem::reload() {

}
void Config_Mem::save() {

}
Config_Mem::Config_Mem() {
//  getcwd(_path, sizeof(_path));
}
