#pragma once

#include <map>
#include <hal/gpio_types.h>
#include <cstdint>

//#define 	BIT(n)   (1UL << (n))
#define    BIT_MASK(n)   (BIT(n) - 1UL)

#define DEBOUNCE_COUNTER_BITS 14
#define DEBOUNCE_COUNTER_MAX BIT_MASK(DEBOUNCE_COUNTER_BITS)

struct debounce_state {
  bool pressed: 1;
  bool changed: 1;
  uint16_t counter: DEBOUNCE_COUNTER_BITS;
};

struct debounce_config {
  /** Duration a switch must be pressed to latch as pressed. */
  uint32_t debounce_press_ms;
  /** Duration a switch must be released to latch as released. */
  uint32_t debounce_release_ms;
};

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
  /**
* Debounces one switch.
*
* @param state The state for the switch to debounce.
* @param active Is the switch currently pressed?
* @param elapsed_ms Time elapsed since the previous update in milliseconds.
* @param config Debounce settings.
*/
  static void key_debounce_update(struct debounce_state *state, bool active, int elapsed_ms,
                           const struct debounce_config *config);

/**
 * @returns whether the switch is either latched as pressed or it is potentially
 * pressed but the debouncer has not yet made a decision. If this returns true,
 * the kscan driver should continue to poll quickly.
 */
  static bool key_debounce_is_active(const struct debounce_state *state);

/**
 * @returns whether the switch is latched as pressed.
 */
  static bool key_debounce_is_pressed(const struct debounce_state *state);

/**
 * @returns whether the pressed state of the switch changed in the last call to
 * debounce_update.
 */
  static bool key_debounce_get_changed(const struct debounce_state *state);

 protected:
  EVENT_STATE last_state[KEY_MAX] = {};
};