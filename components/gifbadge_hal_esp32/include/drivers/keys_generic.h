#pragma once

#include "hal/keys.h"
#include "hal/gpio.h"
class KeysGeneric: public Keys{
 public:
  KeysGeneric(Gpio *up, Gpio *down, Gpio *enter);
  EVENT_STATE *read() override;
  int pollInterval() override;
  void poll();

 private:
  Gpio *_keys[KEY_MAX];
  debounce_state _debounce_states[KEY_MAX];
  debounce_config _debounce_config = {10, 10};
  EVENT_STATE _currentState[KEY_MAX];
  long long last;
  esp_timer_handle_t keyTimer = nullptr;
};