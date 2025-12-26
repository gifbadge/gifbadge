#include <SDL_events.h>
#include "drivers/key_sdl.h"
#include "portable_time.h"
#include "log.h"
#include "hal/keys.h"
#include <array>
#include "FreeRTOS.h"
#include "task.h"

static void keys_refresh(void *args) {
  auto *keys = static_cast<hal::keys::oslinux::keys_sdl *>(args);
  while (true) {
    keys->poll();
    vTaskDelay(30/portTICK_RATE_MS);
  }

}

hal::keys::oslinux::keys_sdl::keys_sdl() {
  _debounce_states[hal::keys::KEY_UP] = hal::keys::debounce_state{false, false, 0};
  _debounce_states[hal::keys::KEY_DOWN] = hal::keys::debounce_state{false, false, 0};
  _debounce_states[hal::keys::KEY_ENTER] = hal::keys::debounce_state{false, false, 0};
  xTaskCreate(keys_refresh, "keys_refresh", 4000, this, 2, &_refreshTask);
}

void hal::keys::oslinux::keys_sdl::poll() {
  std::array<std::pair<SDL_Scancode, hal::keys::EVENT_CODE>, 3> keys = {{
    {SDL_SCANCODE_DOWN, hal::keys::KEY_DOWN},
    {SDL_SCANCODE_UP, hal::keys::KEY_UP},
    {SDL_SCANCODE_SPACE, hal::keys::KEY_ENTER},
}};

  auto time = millis();

  SDL_Event event;

  /* Poll for events. SDL_PollEvent() returns 0 when there are no  */
  /* more events on the event queue, our while loop will exit when */
  /* that occurs.                                                  */
  while (SDL_PollEvent(&event)) {
    /* We are only worried about SDL_KEYDOWN and SDL_KEYUP events */
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      for (auto keycode : keys) {
        if (event.key.keysym.scancode == keycode.first) {
          LOGI("key_sdl", "key %i", keycode.first);
          key_debounce_update(&_debounce_states[keycode.second],
                              event.type == SDL_KEYDOWN,
                              static_cast<int>(time - last),
                              &_debounce_config);
        }
      }
    }
  }
  last = time;
}
hal::keys::EVENT_STATE *hal::keys::oslinux::keys_sdl::read() {
  for (int b = 0; b < KEY_MAX; b++) {
    hal::keys::EVENT_STATE state = key_debounce_is_pressed(&_debounce_states[b]) ? STATE_PRESSED : STATE_RELEASED;
    if (state == STATE_PRESSED && last_state[b] == state) {
      _currentState[b] = STATE_HELD;
    } else {
      last_state[b] = state;
      _currentState[b] = state;
    }
  }
  return _currentState;
}
