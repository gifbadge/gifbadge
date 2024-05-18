#pragma once


#include "hal/keys.h"

class keys_sdl : public Keys {
 public:
  keys_sdl();
  ~keys_sdl() override = default;

  EVENT_STATE * read() override;

  int pollInterval() override { return 100; };

  void poll();

 private:

  debounce_state _debounce_states[KEY_MAX];
  debounce_config _debounce_config = {0, 0};
  EVENT_STATE _currentState[KEY_MAX];


  long last;

};