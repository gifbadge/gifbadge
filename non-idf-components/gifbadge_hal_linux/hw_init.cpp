#include "boards/board_linux.h"

static Board *global_board;

Board *get_board() {
  if (!global_board) {
    global_board = new board_linux();
  }
  return global_board;
}
