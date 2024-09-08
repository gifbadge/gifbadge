#pragma once

#include <stdint.h>

class Gpio{
 public:
  enum class gpio_pull_mode {
    NONE,
    UP,
    DOWN
  };
  enum class gpio_direction {
    IN,
    OUT
  };

  Gpio() = default;
  virtual void config(gpio_direction direction, gpio_pull_mode pull) = 0;
  virtual bool read() = 0;
  virtual void write(bool) = 0;
};