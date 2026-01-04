/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

#pragma once

#include <soc/gpio_num.h>
#include "hal/vbus.h"

namespace hal::vbus::esp32s3 {

class VbusGpio: public hal::vbus::Vbus{
 public:
  VbusGpio(gpio_num_t gpio);
  uint16_t VbusMaxCurrentGet() override;
  void VbusMaxCurrentSet(uint16_t mA) override;
  bool VbusConnected() override;

 private:
  gpio_num_t _gpio;

};
}

