#include "board.h"

/* Unimplemented Yet */
void parse_pgn_move(const char *move, int *fr, int *fc, int *tr, int *tc) {
  (void)move;

  SDL_Log("Move: %s", move);

  *fr = 6; *fc = 4;
  *tr = 4; *tc = 4;
}