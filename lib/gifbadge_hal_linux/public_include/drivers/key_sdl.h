#pragma once


#include "hal/keys.h"

namespace hal::keys::oslinux {
class keys_sdl : public hal::keys::Keys {
 public:
  keys_sdl();
  ~keys_sdl() override = default;

  hal::keys::EVENT_STATE * read() override;

  int pollInterval() override { return 100; };

  void poll();

 private:

  hal::keys::debounce_state _debounce_states[KEY_MAX];
  hal::keys::debounce_config _debounce_config = {0, 0};
  hal::keys::EVENT_STATE _currentState[KEY_MAX];


  long last;

};
}
