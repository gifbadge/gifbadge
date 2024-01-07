#include "hw_init.h"
#include "hal/boards/board_v0.h"
#include "hal/boards/board_2_1_v0_1.h"


std::shared_ptr<Board> hw_init(){
//    return std::make_shared<board_v0>();
    return std::make_shared<board_2_1_v0_1>();
}