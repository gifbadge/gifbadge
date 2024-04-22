#pragma once

#include <hal/gpio_hal.h>
#include <esp_timer.h>
#include "hal/keys.h"

class keys_gpio : public Keys {
 public:
  keys_gpio(gpio_num_t up, gpio_num_t down, gpio_num_t enter);
  ~keys_gpio() override = default;

  std::map<EVENT_CODE, EVENT_STATE> read() override;

  int pollInterval() override { return 100; };

  void poll();

 private:
  std::map<EVENT_CODE, button_state> states;

  std::map<EVENT_CODE, debounce_state> _debounce_states;
  debounce_config _debounce_config = {10, 10};
  esp_timer_handle_t keyTimer = nullptr;

  std::map<EVENT_CODE, EVENT_STATE> _last_state;

  long long last;

};