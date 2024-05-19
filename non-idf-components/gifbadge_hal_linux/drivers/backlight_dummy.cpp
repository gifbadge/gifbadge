#include "drivers/backlight_dummy.h"

backlight_dummy::backlight_dummy() {

}
void backlight_dummy::state(bool) {

}
void backlight_dummy::setLevel(int level) {
  lastLevel = level;
}
int backlight_dummy::getLevel() {
  return lastLevel;
}
