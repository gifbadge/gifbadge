#pragma once
#include <cstdint>
#include <string>
#include <map>
#include <hal/gpio_types.h>

#define KEY_POLL_INTERVAL 100

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

void input_init(QueueHandle_t);

//void input_task(void *arg);

void get_event(input_event i);

std::map<EVENT_CODE, EVENT_STATE> input_read();
