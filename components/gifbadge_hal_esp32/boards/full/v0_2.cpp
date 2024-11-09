#include "boards/full/v0_2.h"

static const char *TAG = "board_2_1_v0_2";

namespace Boards {

esp32::s3::full::v0_2::v0_2() {
  _battery->BatteryInserted(); //Set battery inserted, as we can't detect status on this revision
}

const char *esp32::s3::full::v0_2::Name() {
  return "2.1\" 0.2-0.3";
}
}