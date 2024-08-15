#include "hal/board.h"
static const char *TAG = "HW_INIT";

static Boards::Board *global_board;

#include "esp_efuse_custom_table.h"
#include "boards/boards.h"
#include "boards/v0.h"
#include "boards/b1_28_v0_1.h"
#include "boards/b2_1_v0_2.h"
#include "boards/b2_1_v0_4.h"
#include "boards/b1_28_v0_3.h"

Boards::Board *get_board() {
//    return new board_v0();
  uint8_t board;
  esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA_BOARD, &board, 8);
//    LOGI(TAG, "Board %u", board);
  if (!global_board) {
    switch (board) {
      case BOARD_1_28_V0:
        global_board = new Boards::v0();
        break;
      case BOARD_2_1_V0_2:
        global_board = new Boards::b2_1_v0_2();
        break;
      case BOARD_1_28_V0_1:
        global_board = new Boards::b1_28_v0_1();
        break;
      case BOARD_2_1_V0_4:
        global_board = new Boards::b2_1_v0_4();
        break;
      case BOARD_1_28_V0_3:
        global_board = new Boards::b1_28_v0_3();
        break;
      default:
        return nullptr;
    }
  }
  return global_board;
}
