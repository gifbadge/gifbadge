#pragma once
#include <cstdint>
#include <string>
#include <map>
#include <hal/gpio_types.h>
#include <memory>
#include "hal/keys.h"

struct input_event {
    int64_t timestamp;
    EVENT_CODE code;
    EVENT_STATE value;
};

struct keyArgs {
    Keys *keys;
    QueueHandle_t queue;
};

void input_init(const std::shared_ptr<Keys> &keys, QueueHandle_t);

void get_event(input_event i);

std::map<EVENT_CODE, EVENT_STATE> input_read();
