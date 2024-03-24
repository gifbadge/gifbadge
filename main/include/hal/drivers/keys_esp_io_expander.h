#pragma once

#include <hal/gpio_hal.h>
#include <memory>
#include "hal/keys.h"
#include "esp_io_expander.h"

class keys_esp_io_expander: public Keys{
 public:
  keys_esp_io_expander(esp_io_expander_handle_t io_expander, int up, int down, int enter);
  ~keys_esp_io_expander() override = default;

  std::map<EVENT_CODE, EVENT_STATE> read() override;

  int pollInterval() override {return 100;};
 private:
  esp_io_expander_handle_t _io_expander;
  std::map<EVENT_CODE, int> states;
  uint32_t lastLevels = 0;

};
