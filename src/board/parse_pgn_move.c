#include "board.h"
#include "engine.h"  /* make_bitboards_from_charboard() */

/* Not Done Yet */
void parse_pgn_move(const char *move, SF_Context *sf_ctx, char (*last_pos)[8], int *fr, int *fc, int *tr, int *tc) {
  Turn turn         = sf_ctx->search_color;
  uint8_t castling  = sf_ctx->castling_rights;
  Square en_passant = sf_ctx->enpassant_sq;
  int from_row      = -1;
  int from_col      = -1;
  int to_row        = -1;
  int to_col        = -1;
  char piece_type   = -1;

  const char *ptr = move;

  /* 1 */
  switch (*ptr) {
  case 'N': case 'B': case 'R': case 'Q': case 'K':
    if (turn == WHITE)
      piece_type = *ptr;
    else
      piece_type = *ptr + 32;

    ptr += 1;
    break;

  default:
    if (*ptr >= 'a' && *ptr <= 'h') piece_type = (turn == WHITE) ? 'P' : 'p';
    else return;
    break;
  }

  if (piece_type == -1)
    return;

  /* 2 */
  char file = -1, rank = -1;

  while (*ptr) {
    if (*ptr >= 'a' && *ptr <= 'h' && *(ptr + 1) >= '1' && *(ptr + 1) <= '8') {
      file = *ptr;
      rank = *(ptr + 1);
      break;
    }
    ptr++;
  }

  if (file == -1 || rank == -1)
    return;

  to_row = 7 - (rank - '1');
  to_col = file - 'a';

  /* 3 */
  bool found_src_sq = false;
  MoveList valids   = sf_generate_moves(sf_ctx);

  for (int i = 0; i < valids.count; ++i) {
    Move m        = valids.moves[i];
    Square src_sq = move_from(m);
    Square dst_sq = move_to(m);
    int fr        = square_to_row(src_sq);
    int fc        = square_to_col(src_sq);
    int tr        = square_to_row(dst_sq);
    int tc        = square_to_col(dst_sq);

    bool piece_types_match = last_pos[fr][fc] == piece_type;
    bool destination_match = (tr == to_row && tc == to_col);

    if (piece_types_match && destination_match) {
      from_row     = square_to_row(src_sq);
      from_col     = square_to_col(src_sq);
      found_src_sq = true;
      break;
    }
  }

  if (!found_src_sq) return;

  /* Conclude */
  sf_ctx->bitboard_set    = make_bitboards_from_charboard((const char (*)[8]) last_pos);
  sf_ctx->search_color    = !turn;
  sf_ctx->castling_rights = castling;
  sf_ctx->enpassant_sq    = en_passant;

  *fr = from_row; *fc = from_col;
  *tr = to_row;   *tc = to_col;
}



/*
  
  ** Basic Ideas **
  
  @description: Brief outline to conceptualize the parsing algorithm

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
  
*/