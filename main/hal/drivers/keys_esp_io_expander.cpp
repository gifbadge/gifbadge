#include "hal/drivers/keys_esp_io_expander.h"

keys_esp_io_expander::keys_esp_io_expander(esp_io_expander_handle_t io_expander, int up, int down, int enter): _io_expander(io_expander){


  states.emplace(KEY_UP, up);
  states.emplace(KEY_DOWN, down);
  states.emplace(KEY_ENTER, enter);
}
std::map<EVENT_CODE, EVENT_STATE> keys_esp_io_expander::read() {

  uint32_t levels = lastLevels;
  if(esp_io_expander_get_level(_io_expander, 0xffff, &levels) == ESP_OK){
    //only update when we have a good read
    lastLevels = levels;
  }

  std::map<EVENT_CODE, EVENT_STATE> current_state;
  for (auto &button : states) {
    if (button.second >= 0) {
      bool state = levels&(1<<button.second);
      current_state[button.first] = state ?  STATE_RELEASED:STATE_PRESSED;
    }
  }
  return current_state;
}

