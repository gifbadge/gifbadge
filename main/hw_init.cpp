#include "hw_init.h"
#include "hal/boards/board_v0.h"
#include "hal/boards/board_2_1_v0_1.h"
#include "hal/boards/board_2_1_v0_2.h"
#include "hal/boards/board_1_28_v0_1.h"
#include "esp_efuse_custom_table.h"
#include "hal/boards/boards.h"

static const char *TAG = "HW_INIT";

static std::shared_ptr<Board> global_board;


std::shared_ptr<Board> get_board(){
//    return std::make_shared<board_v0>();
    uint8_t board;
    esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA_BOARD, &board, 8);
    ESP_LOGI(TAG, "Board %u", board);
    if(!global_board) {
        switch (board) {
            case BOARD_1_28_V0:
                global_board = std::make_shared<board_v0>();
                break;
            case BOARD_2_1_V0_2:
              global_board = std::make_shared<board_2_1_v0_2>();
            break;
            case BOARD_1_28_V0_1:
                global_board = std::make_shared<board_1_28_v0_1>();
                break;
            default:
                return nullptr;
        }
    }
    return global_board;
}