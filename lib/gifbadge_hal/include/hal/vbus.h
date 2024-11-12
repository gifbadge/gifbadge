#pragma once

#include <cstdint>

namespace hal::vbus {
/**
 * Abstracts hardware controlling USB VBus supply
 */
class Vbus {
 public:
  Vbus() = default;
  virtual ~Vbus() = default;

  /**
   * The maximum current that can be sourced from USB Host
   * @return current in mA
   */
  virtual uint16_t VbusMaxCurrentGet() = 0;

  /**
   * Set a current limit for what the device can source from the USB Host
   * @param mA current limit in mA
   */
  virtual void VbusMaxCurrentSet(uint16_t mA) = 0;

  /**
   * Check if USB VBus is connected
   * @return true if connected
   */
  virtual bool VbusConnected() = 0;
};
}

