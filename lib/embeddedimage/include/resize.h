#pragma once

#include "fpm/include/fpm/fixed.hpp"
#include "fpm/include/fpm/math.hpp"

using fixedpt = fpm::fixed<std::int32_t, std::int64_t, 12>;
using weight = fpm::fixed<std::int32_t,  std::int64_t, 12>;

typedef struct
{
  int line = -1;
  uint16_t data[2000] = {};
} rowdata;

class Resize {
  private:
  uint16_t* output;
  uint16_t input_width;
  uint16_t input_height;
  uint16_t output_width;
  uint16_t output_height;
  fixedpt ratio;
  int output_y = 0;
  std::array<rowdata, 2> rows = {{{-1,}, {-1,}}};
  public:
  Resize(uint16_t _input_width, uint16_t _input_height, uint16_t _output_width, uint16_t _output_height, uint16_t* _output);

  [[nodiscard]] std::pair<int, int> calc_needed_rows(int y) const;
  void line(int input_y, const uint16_t *buf);
};