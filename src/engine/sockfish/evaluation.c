#include "sockfish/evaluation.h"
#include "sockfish/bitboard.h"
#include "sockfish/movegen.h"

#define calc_color_offset(t) (t==0 ? +1 : -1)
#define flip(s) (s ^ 56)

static int calc_material_score(const SF_Context *ctx);
static int calc_positional_score(const SF_Context *ctx);
static int calc_mobility_score(const SF_Context *ctx);
static int calc_king_safety(const SF_Context *ctx);

int sf_evaluate_position(const SF_Context *ctx) {
  int material_score    = calc_material_score(ctx);
  int positional_score  = calc_positional_score(ctx);
  int mobility_score    = calc_mobility_score(ctx);
  int king_safety_score = calc_king_safety(ctx);
  int color_offset      = calc_color_offset(ctx->search_color);

  int eval = (material_score + positional_score + mobility_score + king_safety_score);
  return eval * color_offset;
}

static int calc_material_score(const SF_Context *ctx) {
  int score = 0;

  const BitboardSet *bbset = &ctx->bitboard_set;

  score += COUNT_BITS(bbset->pawns  [WHITE]) * PAWN_VALUE;
  score += COUNT_BITS(bbset->knights[WHITE]) * KNIGHT_VALUE;
  score += COUNT_BITS(bbset->bishops[WHITE]) * BISHOP_VALUE;
  score += COUNT_BITS(bbset->rooks  [WHITE]) * ROOK_VALUE;
  score += COUNT_BITS(bbset->queens [WHITE]) * QUEEN_VALUE;
  score += COUNT_BITS(bbset->kings  [WHITE]) * KING_VALUE;
  score -= COUNT_BITS(bbset->pawns  [BLACK]) * PAWN_VALUE;
  score -= COUNT_BITS(bbset->knights[BLACK]) * KNIGHT_VALUE;
  score -= COUNT_BITS(bbset->bishops[BLACK]) * BISHOP_VALUE;
  score -= COUNT_BITS(bbset->rooks  [BLACK]) * ROOK_VALUE;
  score -= COUNT_BITS(bbset->queens [BLACK]) * QUEEN_VALUE;
  score -= COUNT_BITS(bbset->kings  [BLACK]) * KING_VALUE;

  return score;
}

static int calc_positional_score(const SF_Context *ctx) {
  int score = 0;

  const BitboardSet *bbset = &ctx->bitboard_set;
  U64 pawns;
  U64 knights;
  U64 bishops;
  U64 rooks;
  U64 queens;
  U64 kings;

  bool no_queens = COUNT_BITS(bbset->queens[WHITE]) == 0 && COUNT_BITS(bbset->queens[BLACK]) == 0;
  const int *KING_TABLE_ = no_queens ? KING_END_GAME_TABLE : KING_TABLE;

  pawns = bbset->pawns[WHITE];
  while (pawns) {
    int sq = POP_LSB(&pawns);
    score += PAWN_TABLE[sq];
  }

  pawns = bbset->pawns[BLACK];
  while (pawns) {
    int sq = POP_LSB(&pawns);
    score -= PAWN_TABLE[flip(sq)];
  }

  knights = bbset->knights[WHITE];
  while (knights) {
    int sq = POP_LSB(&knights);
    score += KNIGHT_TABLE[sq];
  }

  knights = bbset->knights[BLACK];
  while (knights) {
    int sq = POP_LSB(&knights);
    score -= KNIGHT_TABLE[flip(sq)];
  }

  bishops = bbset->bishops[WHITE];
  while (bishops) {
    int sq = POP_LSB(&bishops);
    score += BISHOP_TABLE[sq];
  }

  bishops = bbset->bishops[BLACK];
  while (bishops) {
    int sq = POP_LSB(&bishops);
    score -= BISHOP_TABLE[flip(sq)];
  }

  rooks = bbset->rooks[WHITE];
  while (rooks) {
    int sq = POP_LSB(&rooks);
    score += ROOK_TABLE[sq];
  }

  rooks = bbset->rooks[BLACK];
  while (rooks) {
    int sq = POP_LSB(&rooks);
    score -= ROOK_TABLE[flip(sq)];
  }

  queens = bbset->queens[WHITE];
  while (queens) {
    int sq = POP_LSB(&queens);
    score += QUEEN_TABLE[sq];
  }

  queens = bbset->queens[BLACK];
  while (queens) {
    int sq = POP_LSB(&queens);
    score -= QUEEN_TABLE[flip(sq)];
  }

  kings = bbset->kings[WHITE];
  while (kings) {
    int sq = POP_LSB(&kings);
    score += KING_TABLE_[sq];
  }

  kings = bbset->kings[BLACK];
  while (kings) {
    int sq = POP_LSB(&kings);
    score -= KING_TABLE_[flip(sq)];
  }

  /* ++ Bishop Pair ++  */
  if (COUNT_BITS(bbset->bishops[WHITE]) >= 2) score += 50;
  if (COUNT_BITS(bbset->bishops[BLACK]) >= 2) score -= 50;

  return score;
}

static int calc_mobility_score(const SF_Context *ctx) {
  int mobility_score;
  int white_mobility = 0;
  int black_mobility = 0;

  const BitboardSet *bbset = &ctx->bitboard_set;

  U64 occupied        = bbset->occupied;
  U64 friendly_whites = bbset->all_pieces[WHITE];
  U64 friendly_blacks = bbset->all_pieces[BLACK];
  U64 w_knights       = bbset->knights[WHITE];
  U64 w_bishops       = bbset->bishops[WHITE];
  U64 w_rooks         = bbset->rooks[WHITE];
  U64 b_knights       = bbset->knights[BLACK];
  U64 b_bishops       = bbset->bishops[BLACK];
  U64 b_rooks         = bbset->rooks[BLACK];

  while (w_knights) {
    Square sq       = POP_LSB(&w_knights);
    U64 attacks     = knight_attacks[sq] & ~friendly_whites;
    white_mobility += COUNT_BITS(attacks);
  }

  while (w_bishops) {
    Square sq       = POP_LSB(&w_bishops);
    U64 attacks     = get_bishop_attacks(sq, occupied) & ~friendly_whites;
    white_mobility += COUNT_BITS(attacks);
  }

  while (w_rooks) {
    Square sq       = POP_LSB(&w_rooks);
    U64 attacks     = get_rook_attacks(sq, occupied) & ~friendly_whites;
    white_mobility += COUNT_BITS(attacks);
  }

  while (b_knights) {
    Square sq       = POP_LSB(&b_knights);
    U64 attacks     = knight_attacks[sq] & ~friendly_blacks;
    black_mobility += COUNT_BITS(attacks);
  }

  while (b_bishops) {
    Square sq       = POP_LSB(&b_bishops);
    U64 attacks     = get_bishop_attacks(sq, occupied) & ~friendly_blacks;
    black_mobility += COUNT_BITS(attacks);
  }

  while (b_rooks) {
    Square sq       = POP_LSB(&b_rooks);
    U64 attacks     = get_rook_attacks(sq, occupied) & ~friendly_blacks;
    black_mobility += COUNT_BITS(attacks);
  }

  mobility_score = (white_mobility - black_mobility) * 2;

  return mobility_score;
}

static int calc_king_safety(const SF_Context *ctx) {
  int score = 0;

  const BitboardSet *bbset = &ctx->bitboard_set;

  bool queens_on_board = (bbset->queens[WHITE] | bbset->queens[BLACK]) != 0;

  for (int color = WHITE; color <= BLACK; ++color) {
    int king_score = 0;
    
    if (bbset->kings[color] == 0) continue; // for safety

    Square ksq = GET_LSB(bbset->kings[color]);
    int k_file = ksq % 8; // 0=A, 7=H

    /* If the king is on the flanks */
    if (k_file <= 2 || k_file >= 5) {
      
      int start_file = (k_file == 0) ? 0 : k_file - 1;
      int end_file   = (k_file == 7) ? 7 : k_file + 1;

      for (int f = start_file; f <= end_file; ++f) {
        U64 file_mask = FILE_MASKS[f];

        int our_pawns   = COUNT_BITS(bbset->pawns[color]  & file_mask);
        int enemy_pawns = COUNT_BITS(bbset->pawns[!color] & file_mask);

        if (our_pawns == 0)
          king_score -= 30;
        if (enemy_pawns == 0)
          king_score -= 20;
        if (our_pawns == 0 && enemy_pawns == 0)
          king_score -= 15;
      }
    }

    /* If the king is in the center */
    else if (queens_on_board) king_score -= 50; 

    if (color == WHITE) score += king_score;
    else                score -= king_score;
  }

  return score;
}
