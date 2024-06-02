#include "boards/board_2_1_v0_2.h"

static const char *TAG = "board_2_1_v0_2";

board_2_1_v0_2::board_2_1_v0_2() {
  _battery->inserted(); //Set battery inserted, as we can't detect status on this revision
}

const char * board_2_1_v0_2::name() {
  return "2.1\" 0.2-0.3";
}