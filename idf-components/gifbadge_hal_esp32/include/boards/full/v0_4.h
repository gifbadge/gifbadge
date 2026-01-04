/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once
#include "../../drivers/battery_max17048.h"
#include "../../drivers/keys_esp_io_expander.h"

#include "v0_2_v0_4_common.h"

namespace Boards::esp32::s3::full {
class v0_4 : public Boards::esp32::s3::full::v0_2v0_4 {
  public:
    void LateInit() override;

    v0_4() = default;
  ~v0_4() override = default;

  const char * Name() override;

 private:
  typedef struct {
    hal::battery::esp32s3::battery_max17048 * battery;
    esp_io_expander_handle_t io_expander;
  } batteryTimerArgs;

  batteryTimerArgs _batteryTimerArgs{};
  static bool checkBatteryInstalled(esp_io_expander_handle_t io_expander);
  static void batteryTimer(void *args);
};
}