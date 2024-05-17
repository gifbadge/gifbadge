#pragma once

#include "../../../../../../esp/esp-idf/components/hal/include/hal/gpio_hal.h"
#include "../../../../../../.espressif/tools/xtensa-esp-elf/esp-13.2.0_20230928/xtensa-esp-elf/xtensa-esp-elf/include/c++/13.2.0/memory"
#include "../../keys.h"
#include "../../../../../managed_components/espressif__esp_io_expander/include/esp_io_expander.h"

class keys_esp_io_expander : public Keys {
 public:
  keys_esp_io_expander(esp_io_expander_handle_t io_expander, int up, int down, int enter);
  ~keys_esp_io_expander() override = default;

  EVENT_STATE * read() override;

  int pollInterval() override { return 50; };

  void poll();

 private:
  esp_io_expander_handle_t _io_expander;
  int buttonConfig[KEY_MAX] = {};
  uint32_t lastLevels = 0;

  debounce_state _debounce_states[KEY_MAX] = {};
  debounce_config _debounce_config = {10, 10};
  esp_timer_handle_t keyTimer = nullptr;
  EVENT_STATE _currentState[KEY_MAX] = {};

  long long last;

};
