#pragma once

#include <memory>
#include <hal/keys.h>
#include "esp_io_expander.h"
#include <esp_timer.h>
#include "i2c.h"

namespace hal::keys::esp32s3 {
class keys_esp_io_expander : public hal::keys::Keys {
 public:
  keys_esp_io_expander(esp_io_expander_handle_t io_expander,
                       I2C *i2c,
                       int up,
                       int down,
                       int enter);
  ~keys_esp_io_expander() override = default;

  EVENT_STATE * read() override;

  int pollInterval() override { return 50; };

  void poll();

 private:
  esp_io_expander_handle_t _io_expander;
  I2C *_i2c;
  int buttonConfig[KEY_MAX] = {};
  uint32_t lastLevels = 0;

  debounce_state _debounce_states[KEY_MAX] = {};
  debounce_config _debounce_config = {10, 10};
  esp_timer_handle_t keyTimer = nullptr;
  EVENT_STATE _currentState[KEY_MAX] = {};

  long long last;

};
}

