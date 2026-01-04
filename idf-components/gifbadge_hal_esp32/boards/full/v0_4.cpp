/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#include <driver/sdmmc_defs.h>
#include "boards/full/v0_4.h"
#include "esp_io_expander.h"

static const char *TAG = "board_2_1_v0_4";

namespace Boards{

bool esp32::s3::full::v0_4::checkBatteryInstalled(esp_io_expander_handle_t io_expander) {
  uint32_t levels;
  esp_io_expander_get_level(io_expander, 0xffff, &levels);
  if (levels & (IO_EXPANDER_PIN_NUM_11)) {
    return false;
  }
  return true;
}

void esp32::s3::full::v0_4::batteryTimer(void *args) {
  auto data = static_cast<batteryTimerArgs *>(args);
  if (checkBatteryInstalled(data->io_expander)) {
    data->battery->BatteryInserted();
  } else {
    data->battery->BatteryRemoved();
  }
}

void esp32::s3::full::v0_4::LateInit() {
  v0_2v0_4::LateInit();
  _batteryTimerArgs.battery = _battery;
  _batteryTimerArgs.io_expander = _io_expander;

  batteryTimer(&_batteryTimerArgs);

  const esp_timer_create_args_t batteryTimerSettings =
      {.callback = &batteryTimer, .arg = &_batteryTimerArgs, .dispatch_method = ESP_TIMER_TASK, .name = "battery_check", .skip_unhandled_events = true};
  esp_timer_handle_t batteryTimerHandle = nullptr;
  ESP_ERROR_CHECK(esp_timer_create(&batteryTimerSettings, &batteryTimerHandle));
  ESP_ERROR_CHECK(esp_timer_start_periodic(batteryTimerHandle, 500 * 1000));

}

const char *esp32::s3::full::v0_4::Name() {
  return "2.1\" 0.4";
}
}