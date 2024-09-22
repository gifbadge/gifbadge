#pragma once

#include <cstdint>

class Vbus {
 public:
  Vbus() = default;
  virtual ~Vbus() = default;

  virtual uint16_t VbusMaxCurrentGet() = 0;
  virtual void VbusMaxCurrentSet(uint16_t mA) = 0;

  virtual bool VbusConnected() = 0;
};