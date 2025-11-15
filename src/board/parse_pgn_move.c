#include "board.h"
#include "engine.h"  /* make_bitboards_from_charboard() */

static void take_care_of_pawn_promotes(const char *pgnmove, bool *pawn_promotes, char *promoted_to);

void parse_pgn_move(const char *pgnmove, SF_Context *sf_ctx, char (*last_pos)[8], char *promote, int *fr, int *fc, int *tr, int *tc) {
  Turn turn          = sf_ctx->search_color;
  uint8_t castling   = sf_ctx->castling_rights;
  int from_row       = -1;
  int from_col       = -1;
  int to_row         = -1;
  int to_col         = -1;
  char piece_type    = -1;
  int disambig_file  = -1;
  int disambig_rank  = -1;
  char which_pawn    = -1;
  char promoted_to   = -1;
  bool pawn_promotes = false;

  const char *ptr = pgnmove;

  /* 0 */
  bool queenside = SDL_strstr(pgnmove, "O-O-O") != NULL ||
                   SDL_strstr(pgnmove, "0-0-0") != NULL;

  bool kingside  = SDL_strstr(pgnmove, "O-O") != NULL   ||
                   SDL_strstr(pgnmove, "0-0") != NULL;

  if (queenside || kingside) {
    bool white = turn == WHITE;

    if (white) {
      if (queenside && !(castling & CASTLE_WQ)) return;
      if (kingside  && !(castling & CASTLE_WK)) return;

      castling  &= ~(CASTLE_WK | CASTLE_WQ);
      piece_type = 'K';
      from_row   = 7; from_col = 4; to_row = 7; to_col = queenside ? 2 : 6;
    }
    
    else {
      if (queenside && !(castling & CASTLE_BQ)) return;
      if (kingside  && !(castling & CASTLE_BK)) return;

      castling  &= ~(CASTLE_BK | CASTLE_BQ);
      piece_type = 'k';
      from_row   = 0; from_col = 4; to_row = 0; to_col = queenside ? 2 : 6;
    }

    goto validation;
  }

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
    if (*ptr >= 'a' && *ptr <= 'h') {
      piece_type = (turn == WHITE) ? 'P' : 'p';
      which_pawn = *ptr;
    } else return;
    break;
  }

  const char *disambig_scan_checkpoint_ptr = ptr;

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

  /* 2.5 */
  if (which_pawn != -1) {
    take_care_of_pawn_promotes(pgnmove, &pawn_promotes, &promoted_to);
    goto validation;
  }

  {
    const char *s = disambig_scan_checkpoint_ptr;
    while (s < ptr) {
      if (*s >= 'a' && *s <= 'h') disambig_file = *s - 'a';
      if (*s >= '1' && *s <= '8') disambig_rank = 7 - (*s - '1');
      s++;
    }
  }

  /* 3 */
  validation: {/* jump point */};

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

    bool pawn_move         = which_pawn != -1;
    bool piece_types_match = last_pos[fr][fc] == piece_type;
    bool destination_match = (tr == to_row && tc == to_col);

    if (pawn_move && (fc + 'a' != which_pawn))     piece_types_match = false;
    if (disambig_file >= 0 && fc != disambig_file) piece_types_match = false;
    if (disambig_rank >= 0 && fr != disambig_rank) piece_types_match = false;

    if (piece_types_match && destination_match) {
      from_row     = square_to_row(src_sq);
      from_col     = square_to_col(src_sq);
      found_src_sq = true;
      break;
    }
  }

  if (!found_src_sq) return;

  /* 4 */
  sf_ctx->bitboard_set    = make_bitboards_from_charboard((const char (*)[8]) last_pos);
  sf_ctx->search_color    = !turn;
  sf_ctx->castling_rights = castling;

  if (pawn_promotes)
    *promote = promoted_to;

  *fr = from_row; *fc = from_col;
  *tr = to_row;   *tc = to_col;
}

static void take_care_of_pawn_promotes(const char *pgnmove, bool *pawn_promotes, char *promoted_to) {
  const char *s = SDL_strchr(pgnmove, '=');

  if (s == NULL)
    return;

  s += 1;

  while (*s == ' ' || *s == '\n') s += 1;
  
  char P = SDL_toupper(*s);

  switch (P) {
  case 'Q': case 'N': case 'B': case 'R': break;
  default: return;
  }

  *promoted_to   = P;
  *pawn_promotes = true;
}

/*
  
  ** Basic Ideas **
  
  @description: Brief outline to conceptualize the parsing algorithm
  
  0. Handle castling moves exceptionally.
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

  4. Update *fr, *fc, *tr, *tc values.
  
  Conclude, I guess.
  
*/