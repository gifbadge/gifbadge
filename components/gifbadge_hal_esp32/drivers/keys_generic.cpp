#include <esp_timer.h>
#include "drivers/keys_generic.h"
#include "log.h"

static const char *TAG = "keys_generic";

static void pollKeys(void *args) {
  auto *keys = (KeysGeneric *) args;
  keys->poll();
}

EVENT_STATE *KeysGeneric::read() {
  for (int b = 0; b < KEY_MAX; b++) {
    if (_keys[b] != nullptr) {
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

int KeysGeneric::pollInterval() {
  return 0;
}

KeysGeneric::KeysGeneric(Gpio *up, Gpio *down, Gpio *enter) {
  _keys[KEY_UP] = up,
  _keys[KEY_DOWN] = down,
  _keys[KEY_ENTER] = enter;

  _debounce_states[KEY_UP] = debounce_state{false, false, 0};
  _debounce_states[KEY_DOWN] = debounce_state{false, false, 0};
  _debounce_states[KEY_ENTER] = debounce_state{false, false, 0};

  last = esp_timer_get_time() / 1000;

  const esp_timer_create_args_t keyTimerArgs = {
      .callback = &pollKeys,
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "key_task",
      .skip_unhandled_events = true
  };

  ESP_ERROR_CHECK(esp_timer_create(&keyTimerArgs, &keyTimer));
//  ESP_ERROR_CHECK(esp_timer_start_periodic(keyTimer, 5 * 1000));

}
void KeysGeneric::poll() {
  auto time = esp_timer_get_time() / 1000;

  for (int b = 0; b < KEY_MAX; b++) {
    if (_keys[b] != nullptr) {
      bool state = _keys[b]->GpioRead();
      key_debounce_update(&_debounce_states[b], state == 0, static_cast<int>(time - last), &_debounce_config);
      if (key_debounce_get_changed(&_debounce_states[b])) {
        LOGI(TAG, "%i changed", b);
      }
    }
  }
  last = time;
}