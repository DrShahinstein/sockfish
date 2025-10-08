#include "board.h"

/* Unimplemented Yet */
void parse_pgn_move(const char *move, Turn color, int *fr, int *fc, int *tr, int *tc) {
  (void)move; (void)color;

  SDL_Log("Move: %s", move);
  SDL_Log("Turn: %d", color);

  *fr = 6; *fc = 4;
  *tr = 4; *tc = 4;
}