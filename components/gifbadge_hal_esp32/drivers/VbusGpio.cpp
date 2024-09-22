#include <driver/gpio.h>
#include "drivers/vbus_gpio.h"

VbusGpio::VbusGpio(gpio_num_t gpio): _gpio(gpio) {

}

uint16_t VbusGpio::VbusMaxCurrentGet() {
  if(gpio_get_level(_gpio)){
    return 500;
  }
  return 0;
}

void VbusGpio::VbusMaxCurrentSet(uint16_t mA) {

}
bool VbusGpio::VbusConnected() {
  return gpio_get_level(_gpio);
}
