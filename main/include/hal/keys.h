#pragma once

#include <map>
#include <string>

struct button_state {
    gpio_num_t pin;
    std::string name;
    bool state;
    int64_t time;
};

enum EVENT_CODE {
    KEY_UP,
    KEY_DOWN,
    KEY_ENTER,
};

enum EVENT_STATE {
    STATE_RELEASED,
    STATE_PRESSED
};


class Keys{
public:
    Keys() = default;
    virtual ~Keys() = default;
    virtual std::map<EVENT_CODE, EVENT_STATE> read() = 0;
    virtual int pollInterval() = 0;
};