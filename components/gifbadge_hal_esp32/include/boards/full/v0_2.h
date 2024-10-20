#pragma once
#include "v0_2_v0_4_common.h"

namespace Boards::esp32::s3::full {

class v0_2 : public Boards::esp32::s3::full::v0_2v0_4 {
 public:
  v0_2();
  ~v0_2() override = default;

  const char * name() override;
};
}