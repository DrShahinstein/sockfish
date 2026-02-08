#include "sockfish/sockfish.h"
#include "sockfish/evaluation.h"
#include "sockfish/bitboard.h"

#define calc_color_offset(t) (t==0 ? +1 : -1)

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
  (void)(ctx);
  return 0;
}

static int calc_mobility_score(const SF_Context *ctx) {
  (void)(ctx);
  return 0;
}
