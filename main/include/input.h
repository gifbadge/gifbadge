#pragma once
#include "hal/keys.h"
#include "hal/board.h"

typedef void (*op)();

typedef struct {
  op press;
  op hold;
} keyCommands;

enum MAIN_STATES {
  MAIN_NONE,
  MAIN_NORMAL,
  MAIN_USB,
  MAIN_LOW_BATT,
  MAIN_OTA,
};


void initInputTimer(Board *board);