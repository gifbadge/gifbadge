#include "drivers/backlight_dummy.h"

hal::backlight::oslinux::backlight_dummy::backlight_dummy() {

}
void hal::backlight::oslinux::backlight_dummy::state(bool) {

}
void hal::backlight::oslinux::backlight_dummy::setLevel(int level) {
  lastLevel = level;
}
int hal::backlight::oslinux::backlight_dummy::getLevel() {
  return lastLevel;
}
