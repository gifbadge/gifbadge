#pragma once
#include "board_esp32s3_2_1_0_2_0_4_common.h"

class board_2_1_v0_2 : public board_2_1_v0_2v0_4 {
 public:
  board_2_1_v0_2();
  ~board_2_1_v0_2() override = default;

  const char * name() override;
};