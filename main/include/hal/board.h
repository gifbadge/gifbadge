#pragma once

#include <memory>
#include "hal/battery.h"
#include "hal/touch.h"
#include "i2c.h"
#include "keys.h"
#include "display.h"
#include "hal/backlight.h"

enum BOARD_POWER {
    BOARD_POWER_NORMAL,
    BOARD_POWER_LOW,
    BOARD_POWER_CRITICAL,
};

class Board {
public:
    Board() = default;
    virtual ~Board() = default;

    virtual std::shared_ptr<Battery> getBattery() = 0;
    virtual std::shared_ptr<Touch> getTouch() = 0;
    virtual std::shared_ptr<I2C> getI2c() = 0;
    virtual std::shared_ptr<Keys> getKeys() = 0;
    virtual std::shared_ptr<Display> getDisplay() = 0;
    virtual std::shared_ptr<Backlight> getBacklight() = 0;
    virtual void powerOff() = 0;
    virtual BOARD_POWER powerState() = 0;
    virtual bool storageReady() = 0;
};