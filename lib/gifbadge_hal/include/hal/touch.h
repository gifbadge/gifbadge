#pragma once

#include <cstdint>
#include <utility>

namespace hal::touch {
class Touch {
 public:
  Touch() = default;

  virtual ~Touch() = default;

  virtual std::pair<int16_t, int16_t> read() = 0;
};
}

