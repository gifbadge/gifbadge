#include "hal/linux/boards/board_linux.h"

board_linux::board_linux() {
  _backlight = new backlight_dummy();
  _battery = new battery_dummy();
  _display = displaySdl;
  _config = new Config_Mem();

}
Battery *board_linux::getBattery() {
  return _battery;
}
Touch *board_linux::getTouch() {
  return nullptr;
}
Keys *board_linux::getKeys() {
  return nullptr;
}
Display *board_linux::getDisplay() {
  return _display;
}
Backlight *board_linux::getBacklight() {
  return _backlight;
}
void board_linux::powerOff() {

}
BOARD_POWER board_linux::powerState() {
  return BOARD_POWER_NORMAL;
}
bool board_linux::storageReady() {
  return true;
}
StorageInfo board_linux::storageInfo() {
  return StorageInfo();
}
const char *board_linux::name() {
  return "Linux";
}
bool board_linux::powerConnected() {
  return true;
}
Config *board_linux::getConfig() {
  return _config;
}
