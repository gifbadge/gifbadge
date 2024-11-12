#pragma once

#include "hal/keys.h"
#include "hal/gpio.h"

namespace hal::keys::esp32s3 {
class KeysGeneric: public hal::keys::Keys{
 public:
  KeysGeneric(gpio::Gpio *up, gpio::Gpio *down, gpio::Gpio *enter);
  EVENT_STATE *read() override;
  int pollInterval() override;
  void poll();

 private:
  gpio::Gpio *_keys[KEY_MAX];
  debounce_state _debounce_states[KEY_MAX];
  debounce_config _debounce_config = {50, 50};
  EVENT_STATE _currentState[KEY_MAX];
  long long last;
  esp_timer_handle_t keyTimer = nullptr;
};
}
