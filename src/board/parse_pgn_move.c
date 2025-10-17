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

  /*
  
    1. Find piece type         ('N'f3, 'Q'g6, 'B'a3, 'e'4, 'e'xb7)
    2. Find destination square (N'f3', Q'g6', B'a3', 'e4', ex'b7')
    3. Find source square
      => 0-0-0-0-0-0-0-0
      => 0-0-0-0-0-0-0-0
      => 0-1-0-0-0-0-1-0   'Nh8' => The knight on g6 is the correct one.
      => 0-0-0-0-0-0-0-0
      => 0-0-0-0-0-0-0-0
      => 0-0-0-0-0-0-0-0
      => 0-0-0-0-0-0-0-0
      => 0-0-0-0-0-0-0-0
      
      This can be done with help of sf_generate_moves()
      => for m in valid_moves:
      =>   if dest(m) == parsed_dest_from_phase2:
      =>     src_square = from(m)

    4. Update *fr, *fc, *tr, *tc values with necessary conversions.
    5. Conclude, I guess.

  Okay, the idea seems to emerge.
  Would anybody read this sometime? It would be interesting. Anyways...
  These days I'm focused on math stuff, as a matter of student.
  I'd get back to this implementation later on. It's fun working on such things...
  Let's put the period. Btw, Goodnight Moon is playing. Kill Bill ost is wonderful. So is the movie.
  
  */

  *fr = from_row; *fc = from_col; // from e2
  *tr = to_row;   *tc = to_col;   // to   e4

  last_pos[*fr][*fc]      = 0;
  last_pos[*tr][*tc]      = piece;
  sf_ctx->search_color    = !turn;
  sf_ctx->castling_rights = castling;
  sf_ctx->enpassant_sq    = en_passant;
}