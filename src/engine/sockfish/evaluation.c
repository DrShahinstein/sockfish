#include "evaluation.h"
#include "bitboard.h"
#include "movegen.h"


#define PENALTY_SEMI_OPEN_FILE 15
#define PENALTY_OPEN_FILE 30


void sf_init_evaluation(SF_Context *ctx) {
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

  mg_white -= evaluate_king_safety(&ctx->bitboard_set, WHITE);
  mg_black -= evaluate_king_safety(&ctx->bitboard_set, BLACK);

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

