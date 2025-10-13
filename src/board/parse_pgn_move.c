#include "board.h"

/* Testing */
void parse_pgn_move(const char *move, SF_Context *sf_ctx, char (*last_pos)[8], int *fr, int *fc, int *tr, int *tc) {
  (void)move; (void)sf_ctx; (void)last_pos;

  Turn t = sf_ctx->search_color;

  *fr = 6; *fc = 4; // from e2
  *tr = 4; *tc = 4; // to   e4

  last_pos[*fr][*fc]      = 0;
  last_pos[*tr][*tc]      = 'P';
  sf_ctx->search_color    = !t;
  sf_ctx->castling_rights = CASTLE_NONE;
  sf_ctx->enpassant_sq    = NO_ENPASSANT;
}