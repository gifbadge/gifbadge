#include <memory>
#include "i2c.h"
#include "touch.h"
#include "ft5x06.h"


static void ft5x06_touchpad_init(std::shared_ptr<I2C> bus){
    // Valid touching detect threshold
    uint8_t out;
    out = 70;
    bus->write_reg(0x38, FT5x06_ID_G_THGROUP, &out, 1);

    // valid touching peak detect threshold
    out = 60;
    bus->write_reg(0x38, FT5x06_ID_G_THPEAK, &out, 1);

    // Touch focus threshold
    out = 16;
    bus->write_reg(0x38, FT5x06_ID_G_THCAL, &out, 1);

    // threshold when there is surface water
    out = 60;
    bus->write_reg(0x38, FT5x06_ID_G_THWATER, &out, 1);

    // threshold of temperature compensation
    out = 10;
    bus->write_reg(0x38, FT5x06_ID_G_THTEMP, &out, 1);

    // Touch difference threshold
    out = 20;
    bus->write_reg(0x38, FT5x06_ID_G_THDIFF, &out, 1);

    // Delay to enter 'Monitor' status (s)
    out = 2;
    bus->write_reg(0x38, FT5x06_ID_G_TIME_ENTER_MONITOR, &out, 1);

    // Period of 'Active' status (ms)
    out = 12;
    bus->write_reg(0x38, FT5x06_ID_G_PERIODACTIVE, &out, 1);

    // Timer to enter 'idle' when in 'Monitor' (ms)
    out = 40;
    bus->write_reg(0x38, FT5x06_ID_G_PERIODMONITOR, &out, 1);
}

void ft5x06_touch_read(std::shared_ptr<I2C> bus, QueueHandle_t touch_queue){
    uint8_t data;
    bus->read_reg(0x38, FT5x06_TOUCH_POINTS, &data, 1);
    if((data & 0x0f)> 0 && (data & 0x0f) < 5) {
        uint8_t tmp[4] = {0};
        bus->read_reg(0x38, FT5x06_TOUCH1_XH, tmp, 4);
//            uint16_t x = ((tmp[0] & 0x0f) << 8) + tmp[1]; //not rotated
//            uint16_t y = ((tmp[2] & 0x0f) << 8) + tmp[3];
        uint16_t y = ((tmp[0] & 0x0f) << 8) + tmp[1]; // rotated
        uint16_t x = ((tmp[2] & 0x0f) << 8) + tmp[3];
        y = 480-y;
        if(x > 0 || y > 0) {
            touch_event event = {esp_timer_get_time(), 0, x, y};
            xQueueSendToBack(touch_queue, &event, 100);
        }
    }
}