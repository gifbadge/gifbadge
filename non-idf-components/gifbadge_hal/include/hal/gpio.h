#pragma once

#include <cstdint>

namespace hal::gpio {
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

class Gpio{
 public:
  Gpio() = default;
  virtual void GpioConfig(GpioDirection direction, GpioPullMode pull) = 0;
  virtual bool GpioRead() = 0;
  virtual void GpioWrite(bool b) = 0;
  virtual void GpioInt(hal::gpio::GpioIntDirection dir, void (*callback)()) {};
};
}
