#pragma once

namespace hal::keys {

enum EVENT_CODE {
  KEY_UP,
  KEY_DOWN,
  KEY_ENTER,
  KEY_MAX,
};

enum EVENT_STATE {
  STATE_RELEASED = 0,
  STATE_PRESSED,
  STATE_HELD,
};

class Keys {
 public:
  Keys() = default;
  virtual ~Keys() = default;
  virtual EVENT_STATE * read() = 0;
  virtual int pollInterval() = 0;

 protected:
  EVENT_STATE last_state[KEY_MAX] = {};
};
}

