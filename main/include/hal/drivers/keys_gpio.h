#pragma once

#include <hal/gpio_hal.h>
#include "hal/keys.h"


class keys_gpio: public Keys{
public:
    keys_gpio(gpio_num_t up, gpio_num_t down, gpio_num_t enter);
    ~keys_gpio() override = default;

    std::map<EVENT_CODE, EVENT_STATE> read() override;

    int pollInterval() override {return 100;};
private:
    std::map<EVENT_CODE, button_state> states;
};