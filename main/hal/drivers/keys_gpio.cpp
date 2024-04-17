
#include <driver/gpio.h>
#include <esp_log.h>
#include "hal/drivers/keys_gpio.h"

static const char *TAG = "keys_gpio.cpp";

keys_gpio::keys_gpio(gpio_num_t up, gpio_num_t down, gpio_num_t enter) {
  states.emplace(KEY_UP, (button_state) {up, "up", false, 0});
  states.emplace(KEY_DOWN, (button_state) {down, "down", false, 0});
  states.emplace(KEY_ENTER, (button_state) {enter, "enter", false, 0});

  for (auto &button : states) {
    gpio_num_t gpio = button.second.pin;
    if (gpio >= 0) {
      ESP_LOGI(TAG, "Setting up GPIO %u\n", gpio);
      ESP_ERROR_CHECK(gpio_reset_pin(gpio));
      ESP_ERROR_CHECK(gpio_set_direction(gpio, GPIO_MODE_INPUT));
      ESP_ERROR_CHECK(gpio_set_pull_mode(gpio, GPIO_PULLUP_ONLY));
      ESP_ERROR_CHECK(gpio_pullup_en(gpio));
    }
  }
}

std::map<EVENT_CODE, EVENT_STATE> keys_gpio::read() {
  std::map<EVENT_CODE, EVENT_STATE> current_state;
  for (auto &button : states) {
    if (button.second.pin >= 0) {
      bool state = !gpio_get_level(button.second.pin);
      current_state[button.first] = state ? STATE_PRESSED : STATE_RELEASED;
    }
  }
  return current_state;
}
