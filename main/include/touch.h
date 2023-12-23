#pragma once

struct touch_event {
    int64_t timestamp;
    uint8_t point;
    uint16_t x;
    uint16_t  y;
};