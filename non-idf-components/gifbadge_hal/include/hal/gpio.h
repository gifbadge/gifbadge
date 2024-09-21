#pragma once

#include <cstdint>

class Gpio{
 public:
  enum class GpioPullMode {
    NONE,
    UP,
    DOWN
  };
  enum class GpioDirection {
    IN,
    OUT
  };

  enum class GpioIntDirection {
    NONE,
    RISING,
    FALLING,
  };

  Gpio() = default;
  virtual void GpioConfig(GpioDirection direction, GpioPullMode pull) = 0;
  virtual bool GpioRead() = 0;
  virtual void GpioWrite(bool b) = 0;
  virtual void GpioInt(GpioIntDirection dir, void (*callback)()) {};
};