#include "board.h"

/* Unimplemented Yet */
void parse_pgn_move(const char *move, char (*squares)[8], Turn color, int *fr, int *fc, int *tr, int *tc) {
  (void)move; (void)color; (void)squares;

  *fr = 6; *fc = 4;
  *tr = 4; *tc = 4;
}