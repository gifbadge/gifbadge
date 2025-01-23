#include "resize.h"

#include <cstdio>
#include <cstring>

#define RGB_R(X) ((((X)&0xF800)>>11))
#define RGB_G(X) ((((X)&0x07E0)>>5))
#define RGB_B(X) (((X)&0x001F))

static uint16_t interpolate_rgb565(uint16_t pixel_a,
                                   uint16_t pixel_b,
                                   uint16_t pixel_c,
                                   uint16_t pixel_d,
                                   weight x_weight,
                                   weight y_weight) {
  const uint8_t r = static_cast<uint8_t>(
    RGB_R(pixel_a) * (weight{1.0} - x_weight) * (weight{1.0} - y_weight) +
    RGB_R(pixel_b) * x_weight * (weight{1.0} - y_weight) +
    RGB_R(pixel_c) * y_weight * (weight{1.0} - x_weight) +
    RGB_R(pixel_d) * x_weight * y_weight);
  const uint8_t g = static_cast<uint8_t>(
    RGB_G(pixel_a) * (weight{1.0} - x_weight) * (weight{1.0} - y_weight) +
    RGB_G(pixel_b) * x_weight * (weight{1.0} - y_weight) +
    RGB_G(pixel_c) * y_weight * (weight{1.0} - x_weight) +
    RGB_G(pixel_d) * x_weight * y_weight);
  const uint8_t b = static_cast<uint8_t>(
    RGB_B(pixel_a) * (weight{1.0} - x_weight) * (weight{1.0} - y_weight) +
    RGB_B(pixel_b) * x_weight * (weight{1.0} - y_weight) +
    RGB_B(pixel_c) * y_weight * (weight{1.0} - x_weight) +
    RGB_B(pixel_d) * x_weight * y_weight);
  return r << 11 | g << 5 | b;
}

Resize::Resize(uint16_t _input_width,
               uint16_t _input_height,
               uint16_t _output_width,
               uint16_t _output_height,
               uint16_t *_output): output(_output), input_width(_input_width), input_height(_input_height),
                                   output_width(_output_width),
                                   output_height(_output_height) {
  if (input_width > input_height) {
    ratio = static_cast<fixedpt>(input_width) / static_cast<fixedpt>(output_width);
  } else {
    ratio = static_cast<fixedpt>(input_height) / static_cast<fixedpt>(output_height);
  }
  printf("Resizing image from x:%d y:%d to x:%d y:%d\n", input_width, input_height, output_width, output_height);
}

[[nodiscard]] std::pair<int, int> Resize::calc_needed_rows(int y) const {
  int y_l = static_cast<int>(fpm::floor(ratio * y));
  y_l = std::max(0, y_l);
  y_l = std::min(input_height - 1, y_l);
  int y_h = static_cast<int>(fpm::ceil(ratio * y));
  y_h = std::max(0, y_h);
  y_h = std::min(input_height - 1, y_h);
  return std::make_pair(y_l, y_h);
}

void Resize::line(const int input_y, const uint16_t *buf) {
  auto [y_l, y_h] = calc_needed_rows(output_y);

  if (y_l == input_y || y_h == input_y) {
    if (y_l != rows[0].line && y_h != rows[1].line) {
      memcpy(rows[0].data, buf, input_width * 2);
      rows[0].line = input_y;
    } else {
      memcpy(rows[1].data, buf, input_width * 2);
      rows[1].line = input_y;
    }
  }
  if (((rows[0].line != y_l || rows[1].line != y_l) && input_y != y_l) && ((rows[0].line != y_h || rows[1].line !=
    y_h) && input_y != y_h)) {
    return;
  }

  for (int y = output_y; y < output_height; y++) {
    int row_l = -1;
    int row_h = -1;

    auto [y_l, y_h] = calc_needed_rows(y);

    const weight y_weight = static_cast<weight>(ratio * y) - y_l;

    for (int i = 0; auto [line, data] : rows) {
      if (line == y_h) {
        row_h = i;
      }
      if (line == y_l) {
        row_l = i;
      }
      i++;
    }
    if (row_l == -1 || row_h == -1) {
      return;
    }

    for (int x = 0; x < output_width; x++) {
      int x_l = static_cast<int>(fpm::floor(ratio * x));
      x_l = std::max(0, x_l);
      x_l = std::min(input_height - 1, x_l);
      int x_h = static_cast<int>(fpm::ceil(ratio * x));
      x_h = std::max(0, x_h);
      x_h = std::min(input_height - 1, x_h);

      const weight x_weight = static_cast<weight>(ratio * x) - x_l;

      const uint16_t a = rows[row_l].data[x_l];
      const uint16_t b = rows[row_l].data[x_h];
      const uint16_t c = rows[row_h].data[x_l];
      const uint16_t d = rows[row_h].data[x_h];

      output[(y * output_width) + x] = interpolate_rgb565(a, b, c, d, x_weight, y_weight);
    }

    output_y = y + 1;
  }
}
