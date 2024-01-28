#include "hw_init.h"
#include "hal/boards/board_v0.h"
#include "hal/boards/board_2_1_v0_1.h"
#include "esp_efuse_custom_table.h"

static const char *TAG = "HW_INIT";


std::shared_ptr<Board> hw_init(){
//    return std::make_shared<board_v0>();
    uint8_t board;
    esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA_BOARD, &board, 8);
    ESP_LOGI(TAG, "Board %u", board);
    switch(board){
        case 0:
            return std::make_shared<board_v0>();
        case 1:
            return std::make_shared<board_2_1_v0_1>();
        default:
            return nullptr;
    }
}