#pragma once

#include "esp_timer.h"
#include <hal/keys.h>
#include "hal/gpio_hal.h"

class keys_gpio : public Keys {
 public:
  keys_gpio(gpio_num_t up, gpio_num_t down, gpio_num_t enter);
  ~keys_gpio() override = default;

  EVENT_STATE * read() override;

  int pollInterval() override { return 100; };

  void poll();

 private:
  gpio_num_t buttonConfig[KEY_MAX];

  debounce_state _debounce_states[KEY_MAX];
  debounce_config _debounce_config = {10, 10};
  esp_timer_handle_t keyTimer = nullptr;

  EVENT_STATE _currentState[KEY_MAX];


  long long last;

};