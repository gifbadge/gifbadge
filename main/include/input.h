#pragma once
#include <cstdint>
#include <string>
#include <map>
#include <hal/gpio_types.h>

#define KEY_POLL_INTERVAL 10

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


struct input_event {
    int64_t timestamp;
    EVENT_CODE code;
    EVENT_STATE value;
};

struct touch_event {
    int64_t timestamp;
    uint8_t point;
    uint16_t x;
    uint16_t  y;
};

void input_init();

void input_task(void *arg);

void get_event(input_event i);

std::map<EVENT_CODE, EVENT_STATE> input_read();
