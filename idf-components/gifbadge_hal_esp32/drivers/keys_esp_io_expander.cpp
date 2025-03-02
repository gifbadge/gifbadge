#include <esp_timer.h>
#include "log.h"
#include "drivers/keys_esp_io_expander.h"

static const char *TAG = "keys_esp_io_expander";

static void pollKeys(void *args) {
  auto *keys = (hal::keys::esp32s3::keys_esp_io_expander *) args;
  keys->poll();
}

hal::keys::esp32s3::keys_esp_io_expander::keys_esp_io_expander(esp_io_expander_handle_t io_expander, I2C *i2c, int up, int down, int enter)
    : _io_expander(io_expander), _i2c(i2c) {

  buttonConfig[KEY_UP] = up;
  buttonConfig[KEY_DOWN] = down;
  buttonConfig[KEY_ENTER] = enter;

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

hal::keys::EVENT_STATE * hal::keys::esp32s3::keys_esp_io_expander::read() {
  for (int b = 0; b < KEY_MAX; b++) {
    if (buttonConfig[b] >= 0) {
      hal::keys::EVENT_STATE state = key_debounce_is_pressed(&_debounce_states[b]) ? STATE_PRESSED : STATE_RELEASED;
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

void hal::keys::esp32s3::keys_esp_io_expander::poll() {
  const std::lock_guard<std::mutex> lock(_i2c->i2c_lock);
  uint32_t levels = lastLevels;
  if (esp_io_expander_get_level(_io_expander, 0xffff, &levels) == ESP_OK) {
    //only update when we have a good read
    lastLevels = levels;

    auto time = esp_timer_get_time() / 1000;

    for (int b = 0; b < KEY_MAX; b++) {
      if (buttonConfig[b] >= 0) {
        bool state = levels & (1 << buttonConfig[b]);
        key_debounce_update(&_debounce_states[b], state == 0, static_cast<int>(time - last), &_debounce_config);
        if (key_debounce_get_changed(&_debounce_states[b])) {
          LOGI(TAG, "%i changed", b);
        }
      }
    }
    last = time;
  }
}

