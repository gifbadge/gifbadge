#pragma once

#include <memory>
#include "hal/battery.h"
#include "hal/touch.h"
#include "i2c.h"
#include "keys.h"
#include "display.h"

class Board {
public:
    Board() = default;
    virtual ~Board() = default;

    virtual std::shared_ptr<Battery> getBattery() = 0;
    virtual std::shared_ptr<Touch> getTouch() = 0;
    virtual std::shared_ptr<I2C> getI2c() = 0;
    virtual std::shared_ptr<Keys> getKeys() = 0;
    virtual std::shared_ptr<Display> getDisplay() = 0;
};