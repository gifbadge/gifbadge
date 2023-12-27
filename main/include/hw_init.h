#pragma once
#include "hal/battery.h"
#include "hal/drivers/battery_analog.h"
#include "hal/board.h"

std::shared_ptr<Board> hw_init();
