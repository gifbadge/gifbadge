#include "drivers/backlight_dummy.h"

hal::backlight::_linux::backlight_dummy::backlight_dummy() {

}
void hal::backlight::_linux::backlight_dummy::state(bool) {

}
void hal::backlight::_linux::backlight_dummy::setLevel(int level) {
  lastLevel = level;
}
int hal::backlight::_linux::backlight_dummy::getLevel() {
  return lastLevel;
}
