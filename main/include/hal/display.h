#pragma once

#include <utility>
typedef bool (*flushCallback_t)();

class Display {
 public:
  Display() = default;
  virtual ~Display() = default;

  virtual bool onColorTransDone(flushCallback_t) = 0;
  virtual void write(int x_start, int y_start, int x_end, int y_end, const void *color_data) = 0;

  uint8_t *buffer = nullptr;
  std::pair<int16_t, int16_t> size;

};