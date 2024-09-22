#pragma once

#include <soc/gpio_num.h>
#include "hal/vbus.h"
class VbusGpio: public Vbus{
 public:
  VbusGpio(gpio_num_t gpio);
  uint16_t VbusMaxCurrentGet() override;
  void VbusMaxCurrentSet(uint16_t mA) override;
  bool VbusConnected() override;

 private:
  gpio_num_t _gpio;

};

