#include <cassert>
#include "boards/board_linux.h"
#include "backlight.h"
#include "battery.h"
#include "display.h"
#include "keys.h"
#include "touch.h"
#include "backlight_dummy.h"

board_linux::board_linux() {
  _backlight = new hal::backlight::_linux::backlight_dummy();
  assert(_backlight != nullptr);
  _battery = new battery_dummy();
  assert(_battery != nullptr);
  _display = displaySdl;
  assert(_display != nullptr);
  _config = new Config_Mem();
  assert(_config != nullptr);
  _keys = new keys_sdl();
  assert(_keys != nullptr);



}
hal::battery::Battery *board_linux::getBattery() {
  return _battery;
}
hal::touch::Touch *board_linux::getTouch() {
  return nullptr;
}
hal::keys::Keys *board_linux::getKeys() {
  return _keys;
}
hal::display::Display *board_linux::getDisplay() {
  return _display;
}
hal::backlight::Backlight *board_linux::getBacklight() {
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
void board_linux::debugInfo() {

}
bool board_linux::usbConnected() {
  return false;
}
