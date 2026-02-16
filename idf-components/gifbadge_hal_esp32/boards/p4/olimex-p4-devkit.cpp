/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include "boards/p4/olimex-p4-devkit.h"

#include <log.h>

namespace Boards::esp32::p4 {
olimex_p4_devkit::olimex_p4_devkit() {

}

hal::battery::Battery * olimex_p4_devkit::GetBattery() {
  return _battery;
}
hal::touch::Touch * olimex_p4_devkit::GetTouch() {
  return nullptr;
}
hal::keys::Keys * olimex_p4_devkit::GetKeys() {
  return _keys;
}
hal::display::Display * olimex_p4_devkit::GetDisplay() {
  return _display;
}
hal::backlight::Backlight * olimex_p4_devkit::GetBacklight() {
  return _backlight;
}
BoardPower olimex_p4_devkit::PowerState() {
  return BOARD_POWER_NORMAL;
}
bool olimex_p4_devkit::StorageReady() {
  LOGI("", "SD DETECT %d", gpio_get_level(GPIO_NUM_3));
  // return gpio_get_level(GPIO_NUM_3);
  return true;
}
const char * olimex_p4_devkit::Name() {
  return "olimex_p4_devkit";
}
void olimex_p4_devkit::LateInit() {
  buffer = heap_caps_malloc(480 * 480 + 0x6100, MALLOC_CAP_INTERNAL);
  gpio_set_direction(GPIO_NUM_45, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_45, 0);
  gpio_set_direction(GPIO_NUM_3, GPIO_MODE_INPUT);

  _backlight = new hal::backlight::esp32s3::backlight_dummy();
  _display = new hal::display::esp32s3::display_dummy();
  _keys = new hal::keys::esp32s3::keys_gpio(GPIO_NUM_35, GPIO_NUM_11, GPIO_NUM_12);
  _battery = new hal::battery::esp32s3::battery_dummy();
  if (StorageReady()) {
    mount(GPIO_NUM_43, GPIO_NUM_44, GPIO_NUM_39, GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_42, GPIO_NUM_NC, 4, GPIO_NUM_NC);
  }
}
WakeupSource olimex_p4_devkit::BootReason() {
  return WakeupSource::KEY;
}
void olimex_p4_devkit::DebugInfo() {
}
}
