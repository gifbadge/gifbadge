#include "boards/board_linux.h"

static Boards::Board *global_board;

Boards::Board *get_board() {
  if (!global_board) {
    global_board = new board_linux();
  }
  return global_board;
}
