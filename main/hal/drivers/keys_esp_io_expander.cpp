#include <esp_timer.h>
#include <esp_log.h>
#include "hal/drivers/keys_esp_io_expander.h"

static const char *TAG = "keys_esp_io_expander";

static void pollKeys(void *args) {
  auto *keys = (keys_esp_io_expander *) args;
  keys->poll();
}

keys_esp_io_expander::keys_esp_io_expander(esp_io_expander_handle_t io_expander, int up, int down, int enter)
    : _io_expander(io_expander) {

  states.emplace(KEY_UP, up);
  states.emplace(KEY_DOWN, down);
  states.emplace(KEY_ENTER, enter);

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

std::map<EVENT_CODE, EVENT_STATE> keys_esp_io_expander::read() {

  std::map<EVENT_CODE, EVENT_STATE> current_state;
  for (auto &button : states) {
    if (button.second >= 0) {
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

void keys_esp_io_expander::poll() {
  uint32_t levels = lastLevels;
  if (esp_io_expander_get_level(_io_expander, 0xffff, &levels) == ESP_OK) {
    //only update when we have a good read
    lastLevels = levels;

    auto time = esp_timer_get_time() / 1000;

    std::map<EVENT_CODE, EVENT_STATE> current_state;
    for (auto &button : states) {
      if (button.second >= 0) {
        bool state = levels & (1 << button.second);
        key_debounce_update(&_debounce_states[button.first], state == 0, time - last, &_debounce_config);
        if (key_debounce_get_changed(&_debounce_states[button.first])) {
          ESP_LOGI(TAG, "%i changed", button.first);
        }
      }
    }
    last = time;
  }
}

