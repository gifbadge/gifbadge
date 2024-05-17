
#include <driver/gpio.h>
#include "log.h"
#include "hal/esp32/drivers/keys_gpio.h"

static const char *TAG = "keys_gpio.cpp";

static void pollKeys(void *args) {
  auto *keys = (keys_gpio *) args;
  keys->poll();
}

keys_gpio::keys_gpio(gpio_num_t up, gpio_num_t down, gpio_num_t enter) {
  buttonConfig[KEY_UP] = up;
  buttonConfig[KEY_DOWN] = down;
  buttonConfig[KEY_ENTER] = enter;

  for (auto &input : buttonConfig) {
    if (input >= 0) {
      LOGI(TAG, "Setting up GPIO %u\n", input);
      ESP_ERROR_CHECK(gpio_reset_pin(input));
      ESP_ERROR_CHECK(gpio_set_direction(input, GPIO_MODE_INPUT));
      ESP_ERROR_CHECK(gpio_set_pull_mode(input, GPIO_PULLUP_ONLY));
      ESP_ERROR_CHECK(gpio_pullup_en(input));
    }
  }

  _debounce_states[KEY_UP] = debounce_state{false, false, 0};
  _debounce_states[KEY_DOWN] = debounce_state{false, false, 0};
  _debounce_states[KEY_ENTER] = debounce_state{false, false, 0};

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

EVENT_STATE * keys_gpio::read() {

  for (int b = 0; b < KEY_MAX; b++) {
    if (buttonConfig[b] >= 0) {
      EVENT_STATE state = key_debounce_is_pressed(&_debounce_states[b]) ? STATE_PRESSED : STATE_RELEASED;
      if (state == STATE_PRESSED && last_state[b] == state) {
        _currentState[b] = STATE_HELD;
      } else {
        last_state[b] = state;
        _currentState[b] = state;
      }
    }
  }
  return _currentState;
}

void keys_gpio::poll() {
    auto time = esp_timer_get_time() / 1000;

    for (int b = 0; b < KEY_MAX; b++) {
      if (buttonConfig[b] >= 0) {
        bool state = gpio_get_level(buttonConfig[b]);
        key_debounce_update(&_debounce_states[b], state == 0, static_cast<int>(time - last), &_debounce_config);
        if (key_debounce_get_changed(&_debounce_states[b])) {
          LOGI(TAG, "%i changed", b);
        }
      }
    }
    last = time;
}
