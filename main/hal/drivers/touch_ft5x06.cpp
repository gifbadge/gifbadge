#include "hal/drivers/touch_ft5x06.h"

#include <utility>

touch_ft5x06::touch_ft5x06(std::shared_ptr<I2C> bus) : _bus(std::move(bus)) {
  // Valid touching detect threshold
  uint8_t out;
  out = 70;
  _bus->write_reg(0x38, FT5x06_ID_G_THGROUP, &out, 1);

  // valid touching peak detect threshold
  out = 60;
  _bus->write_reg(0x38, FT5x06_ID_G_THPEAK, &out, 1);

  // Touch focus threshold
  out = 16;
  _bus->write_reg(0x38, FT5x06_ID_G_THCAL, &out, 1);

  // threshold when there is surface water
  out = 60;
  _bus->write_reg(0x38, FT5x06_ID_G_THWATER, &out, 1);

  // threshold of temperature compensation
  out = 10;
  _bus->write_reg(0x38, FT5x06_ID_G_THTEMP, &out, 1);

  // Touch difference threshold
  out = 20;
  _bus->write_reg(0x38, FT5x06_ID_G_THDIFF, &out, 1);

  // Delay to enter 'Monitor' status (s)
  out = 2;
  _bus->write_reg(0x38, FT5x06_ID_G_TIME_ENTER_MONITOR, &out, 1);

  // Period of 'Active' status (ms)
  out = 12;
  _bus->write_reg(0x38, FT5x06_ID_G_PERIODACTIVE, &out, 1);

  // Timer to enter 'idle' when in 'Monitor' (ms)
  out = 40;
  _bus->write_reg(0x38, FT5x06_ID_G_PERIODMONITOR, &out, 1);
}

std::pair<int16_t, int16_t> touch_ft5x06::read() {
  uint8_t data;
  _bus->read_reg(0x38, FT5x06_TOUCH_POINTS, &data, 1);
  if ((data & 0x0f) > 0 && (data & 0x0f) < 5) {
    uint8_t tmp[4] = {0};
    _bus->read_reg(0x38, FT5x06_TOUCH1_XH, tmp, 4);
    int16_t x = static_cast<int16_t>((tmp[0] & 0x0f) << 8) + tmp[1]; //not rotated
    int16_t y = static_cast<int16_t>((tmp[2] & 0x0f) << 8) + tmp[3];
//        uint16_t y = ((tmp[0] & 0x0f) << 8) + tmp[1]; // rotated
//        uint16_t x = ((tmp[2] & 0x0f) << 8) + tmp[3];
//        y = 480-y;
    if (x > 0 || y > 0) {
      return {x, y};
    }
  }
  return {-1, -1};
}



