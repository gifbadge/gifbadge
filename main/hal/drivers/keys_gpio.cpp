
#include <driver/gpio.h>
#include <esp_log.h>
#include "hal/drivers/keys_gpio.h"

static const char *TAG = "keys_gpio.cpp";

static void pollKeys(void *args) {
  auto *keys = (keys_gpio *) args;
  keys->poll();
}

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

  _last_state.emplace(KEY_UP, STATE_RELEASED);
  _last_state.emplace(KEY_DOWN, STATE_RELEASED);
  _last_state.emplace(KEY_ENTER, STATE_RELEASED);

  _debounce_states.emplace(KEY_UP, debounce_state{false, false, 0});
  _debounce_states.emplace(KEY_DOWN, debounce_state{false, false, 0});
  _debounce_states.emplace(KEY_ENTER, debounce_state{false, false, 0});

  const esp_timer_create_args_t keyTimerArgs = {
      .callback = &pollKeys,
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "key_task",
      .skip_unhandled_events = true
  };

  ESP_ERROR_CHECK(esp_timer_create(&keyTimerArgs, &keyTimer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(keyTimer, 5 * 1000));
  last = esp_timer_get_time() / 1000;

}

std::map<EVENT_CODE, EVENT_STATE> keys_gpio::read() {

  std::map<EVENT_CODE, EVENT_STATE> current_state;
  for (auto &button : states) {
    if (button.second.pin >= 0) {
      current_state[button.first] =
          key_debounce_is_pressed(&_debounce_states[button.first]) ? STATE_PRESSED : STATE_RELEASED;
      if (current_state[button.first] == STATE_PRESSED && _last_state[button.first] == current_state[button.first]) {
        current_state[button.first] = STATE_HELD;
      } else {
        _last_state[button.first] = current_state[button.first];
      }
    }
  }
  return current_state;
}

void keys_gpio::poll() {
    auto time = esp_timer_get_time() / 1000;

    std::map<EVENT_CODE, EVENT_STATE> current_state;
    for (auto &button : states) {
      if (button.second.pin >= 0) {
        bool state = gpio_get_level(button.second.pin);
        key_debounce_update(&_debounce_states[button.first], state == 0, time - last, &_debounce_config);
        if (key_debounce_get_changed(&_debounce_states[button.first])) {
          ESP_LOGI(TAG, "%i changed", button.first);
        }
      }
    }
    last = time;
}