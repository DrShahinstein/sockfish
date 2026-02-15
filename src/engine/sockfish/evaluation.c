#include "sockfish/evaluation.h"
#include "sockfish/bitboard.h"
#include "sockfish/movegen.h"

#define calc_color_offset(t) (t==0 ? +1 : -1)
#define flip(s) (s ^ 56)

static int calc_material_score(const SF_Context *ctx);
static int calc_positional_score(const SF_Context *ctx);
static int calc_mobility_score(const SF_Context *ctx);

int sf_evaluate_position(const SF_Context *ctx) {
  int material_score   = calc_material_score(ctx);
  int positional_score = calc_positional_score(ctx);
  int mobility_score   = calc_mobility_score(ctx);

  int eval = (material_score + positional_score + mobility_score);
  return eval;
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
    score += KING_MIDDLE_GAME_TABLE[sq];
  }

  kings = bbset->kings[BLACK];
  while (kings) {
    int sq = POP_LSB(&kings);
    score -= KING_MIDDLE_GAME_TABLE[flip(sq)];
  }

  if (COUNT_BITS(bbset->bishops[WHITE]) >= 2) score += 50;
  if (COUNT_BITS(bbset->bishops[BLACK]) >= 2) score -= 50;

  return score;
}

static int calc_mobility_score(const SF_Context *ctx) {
  SF_Context white_tmpctx   = *ctx;
  white_tmpctx.search_color = WHITE;
  MoveList white_moves      = generate_pseudo_legal_moves(&white_tmpctx);

  SF_Context black_tmpctx   = *ctx;
  black_tmpctx.search_color = BLACK;
  MoveList black_moves      = generate_pseudo_legal_moves(&black_tmpctx);

  return (white_moves.count - black_moves.count) * 10;
}
