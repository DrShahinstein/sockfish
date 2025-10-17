#include "board.h"

/* Testing */
void parse_pgn_move(const char *move, SF_Context *sf_ctx, char (*last_pos)[8], int *fr, int *fc, int *tr, int *tc) {
  (void)move; (void)sf_ctx; (void)last_pos;

  Turn turn         = sf_ctx->search_color;
  uint8_t castling  = CASTLE_WK & CASTLE_WQ & CASTLE_BK & CASTLE_BQ;
  Square en_passant = NO_ENPASSANT;
  int from_row      = 6;
  int from_col      = 4;
  int to_row        = 4;
  int to_col        = 4;
  char piece;
  //
  
  /* Experimentsssss */
  
  // Let's try to think of an algorithm
  /*
  
  1. Spot the moving piece ('N'f4, 'e'6, 'N'c6, 'Q'h8)
  2. Spot the available moves that the moving piece(s) can do
  => 0-0-0-0-0-0-0-0
     0-0-0-0-0-1-0-0
     0-0-0-0-0-0-0-0
     0-0-0-0-0-0-0-0
     0-0-1-0-0-0-0-0
     0-0-0-0-0-0-0-0
     0-0-0-0-0-0-0-0
     0-0-0-0-0-0-0-0

    * Let's say '1's are knights. And we have 'Nh8' to parse. Only the knight on f7 can make this move.
     So, it's the knight! We would convert F7 and H8 to row_col coordinates one by one.

    * If none can go to h8, pgn is considered invalid.

    * If it's pawn we have, then handle the job with a separate different logic because it's a little different to parse.

    __end

  3. Oh, that's enough of a thinking process.

  It's okay, but, how to exactly find out whether a piece can or cannot go to a certain square?
  These things seems easier when I used to work around with movegen.c implementation. It was magic entries there...
  HMmmmmmmm.. I have to figure this out.
  
  */


  //
  *fr = from_row; *fc = from_col; // from e2
  *tr = to_row;   *tc = to_col;   // to   e4

  last_pos[*fr][*fc]      = 0;
  last_pos[*tr][*tc]      = piece;
  sf_ctx->search_color    = !turn;
  sf_ctx->castling_rights = castling;
  sf_ctx->enpassant_sq    = en_passant;
}