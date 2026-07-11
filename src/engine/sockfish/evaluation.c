#include "evaluation.h"
#include "bitboard.h"
#include "movegen.h"


U64 passed_pawn_masks[2][64];


/* Procedural Functions: Called once and used for clarity */
static inline void king_safety(const SF_Context *ctx, int *mg_white, int *mg_black);
static inline void pawn_structure(const SF_Context *ctx, int *mg_white, int *mg_black, int *eg_white, int *eg_black);


void sf_init_eval_masks(void) {
  for (int sq = 0; sq < 64; ++sq) {
    int r = sq / 8;
    int c = sq % 8;

    U64 w_mask = 0;
    for (int i = r + 1; i < 8; ++i) {
      w_mask |= (1ULL << (i * 8 + c));
      if (c > 0) w_mask |= (1ULL << (i * 8 + c - 1));
      if (c < 7) w_mask |= (1ULL << (i * 8 + c + 1));
    }
    passed_pawn_masks[WHITE][sq] = w_mask;

    U64 b_mask = 0;
    for (int i = r - 1; i >= 0; --i) {
      b_mask |= (1ULL << (i * 8 + c));
      if (c > 0) b_mask |= (1ULL << (i * 8 + c - 1));
      if (c < 7) b_mask |= (1ULL << (i * 8 + c + 1));
    }
    passed_pawn_masks[BLACK][sq] = b_mask;
  }
}


void sf_init_evaluation(SF_Context *ctx) {
  static bool masks_initialized = false;

  if (!masks_initialized) {
    sf_init_eval_masks();
    masks_initialized = true;
  }

  ctx->mg_score[WHITE] = 0;
  ctx->mg_score[BLACK] = 0;
  ctx->eg_score[WHITE] = 0;
  ctx->eg_score[BLACK] = 0;
  ctx->game_phase      = 0;

  const BitboardSet *bbset = &ctx->bitboard_set;
  int phase_values[6] = {PHASE_PAWN, PHASE_KNIGHT, PHASE_BISHOP, PHASE_ROOK, PHASE_QUEEN, 0};

  U64 bitboards[2][6] = {
    {bbset->pawns[WHITE], bbset->knights[WHITE], bbset->bishops[WHITE], bbset->rooks[WHITE], bbset->queens[WHITE], bbset->kings[WHITE]},
    {bbset->pawns[BLACK], bbset->knights[BLACK], bbset->bishops[BLACK], bbset->rooks[BLACK], bbset->queens[BLACK], bbset->kings[BLACK]}
  };

  for (int color = WHITE; color <= BLACK; ++color) {
    for (int piece=0; piece < 6; ++piece) {
      U64 bb = bitboards[color][piece];
      
      while (bb) {
        int sq = POP_LSB(&bb);
        
        // PeSTOs assumes A8=0 but it's A1=0 for us
        int table_sq = (color == WHITE) ? flip(sq) : sq;

        ctx->mg_score[color] += PESTO_TABLE[piece][MG][table_sq] + MG_MATERIAL[piece];
        ctx->eg_score[color] += PESTO_TABLE[piece][EG][table_sq] + EG_MATERIAL[piece];
        ctx->game_phase      += phase_values[piece];
      }
    }
  }
}


int sf_evaluate_position(const SF_Context *ctx) {
  int mg_white = ctx->mg_score[WHITE];
  int mg_black = ctx->mg_score[BLACK];
  int eg_white = ctx->eg_score[WHITE];
  int eg_black = ctx->eg_score[BLACK];

  king_safety(ctx, &mg_white, &mg_black);
  pawn_structure(ctx, &mg_white, &mg_black, &eg_white, &eg_black);

  int mg_score = mg_white - mg_black;
  int eg_score = eg_white - eg_black;

  int phase = ctx->game_phase;
  if (phase > 24) phase = 24;

  /* Tapered Evaluation: Adjusts the evaluation score by the weight of the game phase */
  int eval = (mg_score * phase + eg_score * (24 - phase)) / 24;

  return eval * calc_color_offset(ctx->search_color);
}


int evaluate_king_safety(const BitboardSet *bbs, Turn color) {
  if (bbs->kings[color] == 0) return 0;

  int penalty    = 0;
  Square king_sq = GET_LSB(bbs->kings[color]);
  int king_file  = square_to_col(king_sq);
  Turn opponent  = !color;
  U64 my_pawns   = bbs->pawns[color];
  U64 opp_pawns  = bbs->pawns[opponent];

  int start_file = (king_file > 0) ? king_file - 1 : 0;
  int end_file   = (king_file < 7) ? king_file + 1 : 7;

  for (int f = start_file; f <= end_file; ++f) {
    U64 file_mask = FILE_MASKS[f];

    if (!(my_pawns & file_mask)) {
      if (!(opp_pawns & file_mask))
        penalty += PENALTY_OPEN_FILE;      
      else
        penalty += PENALTY_SEMI_OPEN_FILE; 
    }
  }

  return penalty;
}


void evaluate_pawns(const BitboardSet *bbs, Turn color, int *mg_bonus, int *eg_bonus) {
  U64 my_pawns  = bbs->pawns[color];
  U64 opp_pawns = bbs->pawns[!color];
  
  int mg = 0;
  int eg = 0;

  for (int f = 0; f < 8; ++f) {
    int pawn_count = COUNT_BITS(my_pawns & FILE_MASKS[f]);

    if (pawn_count > 1) {
      mg -= PENALTY_DOUBLED_PAWN * (pawn_count - 1);
      eg -= PENALTY_DOUBLED_PAWN * (pawn_count - 1);
    }
  }

  U64 pawns_copy = my_pawns;

  while (pawns_copy) {
    Square sq          = POP_LSB(&pawns_copy);
    int rank           = sq / 8;
    int file           = sq % 8;
    int relative_rank  = (color == WHITE) ? rank : (7-rank);
    U64 adjacent_files = 0;

    if (file > 0) adjacent_files |= FILE_MASKS[file - 1];
    if (file < 7) adjacent_files |= FILE_MASKS[file + 1];

    if (!(my_pawns & adjacent_files)) {
      mg -= PENALTY_ISOLATED_PAWN;
      eg -= PENALTY_ISOLATED_PAWN;
    }

    if (!(opp_pawns & passed_pawn_masks[color][sq])) {
      mg += PASSED_PAWN_BONUS_MG[relative_rank];
      eg += PASSED_PAWN_BONUS_EG[relative_rank];
    }
  }
  
  *mg_bonus = mg;
  *eg_bonus = eg;
}


/* --- Procedural Functions --- */

static inline void king_safety(const SF_Context *ctx, int *mg_white, int *mg_black) {
  *mg_white -= evaluate_king_safety(&ctx->bitboard_set, WHITE);
  *mg_black -= evaluate_king_safety(&ctx->bitboard_set, BLACK);
}

static inline void pawn_structure(const SF_Context *ctx, int *mg_white, int *mg_black, int *eg_white, int *eg_black) {
  int w_mg_pawns = 0, w_eg_pawns = 0;
  int b_mg_pawns = 0, b_eg_pawns = 0;
  
  evaluate_pawns(&ctx->bitboard_set, WHITE, &w_mg_pawns, &w_eg_pawns);
  evaluate_pawns(&ctx->bitboard_set, BLACK, &b_mg_pawns, &b_eg_pawns);

  *mg_white += w_mg_pawns; *eg_white += w_eg_pawns;
  *mg_black += b_mg_pawns; *eg_black += b_eg_pawns;
}

