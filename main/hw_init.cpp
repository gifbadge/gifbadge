#include "hw_init.h"

static const char *TAG = "HW_INIT";

static Board *global_board;

#ifdef ESP_PLATFORM
#include "hal/esp32/boards/board_v0.h"
#include "hal/esp32/boards/board_2_1_v0_2.h"
#include "hal/esp32/boards/board_1_28_v0_1.h"
#include "hal/esp32/esp_efuse_custom_table.h"
#include "hal/esp32/boards/boards.h"
#include "hal/esp32/boards/board_2_1_v0_4.h"

Board *get_board() {
//    return new board_v0();
  uint8_t board;
  esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA_BOARD, &board, 8);
//    LOGI(TAG, "Board %u", board);
  if (!global_board) {
    switch (board) {
      case BOARD_1_28_V0:
        global_board = new board_v0();
        break;
      case BOARD_2_1_V0_2:
        global_board = new board_2_1_v0_2();
        break;
      case BOARD_1_28_V0_1:
        global_board = new board_1_28_v0_1();
        break;
      case BOARD_2_1_V0_4:
        global_board = new board_2_1_v0_4();
        break;
      default:
        return nullptr;
    }
  }
  return global_board;
}
#else
#include "hal/linux/boards/board_linux.h"

Board *get_board() {
  if (!global_board) {
   global_board = new board_linux();
  }
  return global_board;
}
#endif