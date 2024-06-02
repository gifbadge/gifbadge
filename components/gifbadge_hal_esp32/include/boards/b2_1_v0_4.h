#pragma once
#include "drivers/battery_max17048.h"
#include "drivers/keys_esp_io_expander.h"

#include "b2_1_v0_2_v0_4_common.h"

namespace Boards {
class b2_1_v0_4 : public Boards::b2_1_v0_2v0_4 {
 public:
  b2_1_v0_4();
  ~b2_1_v0_4() override = default;

  const char * name() override;

 private:
  typedef struct {
    battery_max17048 * battery;
    esp_io_expander_handle_t io_expander;
  } batteryTimerArgs;

  batteryTimerArgs _batteryTimerArgs{};
  static bool checkBatteryInstalled(esp_io_expander_handle_t io_expander);
  static void batteryTimer(void *args);
};
}