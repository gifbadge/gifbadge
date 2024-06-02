#pragma once
#include "b2_1_v0_2_v0_4_common.h"

namespace Boards {
class b2_1_v0_2 : public Boards::b2_1_v0_2v0_4 {
 public:
  b2_1_v0_2();
  ~b2_1_v0_2() override = default;

  const char * name() override;
};
}