#include "board.h"

/* Unimplemented Yet */
void parse_pgn_move(const char *move, SF_Context *sf_ctx, char (*squares)[8], int *fr, int *fc, int *tr, int *tc) {
  (void)move; (void)sf_ctx; (void)squares;

  *fr = 6; *fc = 4;
  *tr = 4; *tc = 4;

  squares[*fr][*fc] = 0;
  squares[*tr][*tc] = 'P';
}