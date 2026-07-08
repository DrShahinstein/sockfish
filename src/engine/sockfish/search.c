#include "sockfish/search.h"
#include "sockfish/sockfish.h"
#include "sockfish/evaluation.h"
#include "sockfish/move_helper.h"
#include "sockfish/movegen.h"
#include "sockfish/transposition_table.h"

#include <SDL3/SDL_timer.h>  /* SDL_GetTicks() */
#include <SDL3/SDL.h>        // DEBUG

#define INF 9999999
#define MATE_SCORE 9000000
#define MATE_BOUND 8000000
#define MAX_DEPTH 40
#define SEARCH_TIME 3000  /* ms */

static const int ROOT_PLY=0;
static const int ALLOW_NULL=true;

static inline bool check_time(SF_Context *ctx);
static inline bool is_repetition(const SF_Context *ctx);
static inline int score_to_tt(int score, int ply);
static inline int score_from_tt(int score, int ply);
static inline bool giving_check(Move move, PieceType attacker, const CheckMasks *masks);
static inline bool has_non_pawn_material(const SF_Context *ctx);
static inline int get_lmr_reduction(int depth, int legal_moves, bool is_quiet, bool gives_check, bool in_check);

static int score_move(const SF_Context *ctx, Move move, Move best_so_far, const CheckMasks *masks);
static void bump_highest_scored_move(int i, MoveList *movelist, int *scores);
static CheckMasks generate_check_masks(const SF_Context *ctx);


Move sf_search(const SF_Context *ctx) {
  SF_Context ctx_ = *ctx;
  ctx_.nodes      = 0;
  ctx_.start_time = SDL_GetTicks();
  ctx_.time_limit = SEARCH_TIME;

  /* For Safety */
  bool local_stop = false;
  if (ctx_.should_stop == NULL)
    ctx_.should_stop = &local_stop;

  Move best_move = create_move(A1,A1);

  for (int depth=1; depth <= MAX_DEPTH; ++depth) {
    if (check_time(&ctx_)) break;
    
    int alpha            = -INF;
    int beta             = +INF;
    int max_score_so_far = -INF;
    Move best_so_far     = best_move;

    MoveList movelist = generate_pseudo_legal_moves(&ctx_);
    if (movelist.count == 0) break;

    CheckMasks masks = generate_check_masks(ctx);

    int scores[256]; // scores[i] <===> movelist->moves[i]
    for (int i=0; i < movelist.count; ++i) {
      scores[i] = score_move(&ctx_, movelist.moves[i], best_so_far, &masks);
    }

    for (int i=0; i < movelist.count; ++i) {
      bump_highest_scored_move(i, &movelist, scores);

      MoveHistory history;
      make_move(&ctx_, movelist.moves[i], &history);

      if (king_in_check(&ctx_.bitboard_set, !ctx_.search_color)) {
        unmake_move(&ctx_, &history);
        continue;
      }

      int score = -negamax(&ctx_, depth-1, ROOT_PLY+1, -beta, -alpha, ALLOW_NULL);

      unmake_move(&ctx_, &history);

      if (*ctx_.should_stop) break;

      if (score > max_score_so_far) {
        max_score_so_far = score;
        best_so_far      = movelist.moves[i];
      }

      if (score > alpha) {
        alpha = score;
      }
    }

    if (*ctx_.should_stop) break;

    int tt_record_score = score_to_tt(max_score_so_far, ROOT_PLY);
    tt_record(ctx_.hash_key, depth, tt_record_score, TT_EXACT, best_so_far);

    best_move = best_so_far;
  }

  return best_move;
}

int negamax(SF_Context *ctx, unsigned int depth, int ply, int alpha, int beta, bool allow_null) {
  ctx->nodes++;

  if (check_time(ctx)) return 0;

  if (depth == 0) {
    return quiescence_search(ctx, ply, alpha, beta);
  }

  if (is_repetition(ctx)) {
    return 0; // threefold repetition draw
  }

  if (ctx->history_count >= SF_MAX_HIST)
    return sf_evaluate_position(ctx); // avoid potential stack overflow (shouldn't happen)

  int original_alpha = alpha;
  int tt_score = 0;
  Move tt_move = 0;

  if (tt_probe(ctx->hash_key, depth, alpha, beta, &tt_score, &tt_move)) {
    return score_from_tt(tt_score, ply);
  }

  int static_eval = sf_evaluate_position(ctx);
  bool in_check   = king_in_check(&ctx->bitboard_set, ctx->search_color);
  bool nmp        = allow_null && depth >= 3 && !in_check && has_non_pawn_material(ctx) && static_eval >= beta;

  if (nmp) {
    int nmp_score = null_move_search(ctx, depth, ply, beta);
    
    if (nmp_score != -1)
      return nmp_score; // beta
  }

  MoveList movelist = generate_pseudo_legal_moves(ctx);
  Move best_so_far  = tt_move; // the best result we obtained in previous nodes
  Move best_move    = 0;       // the best result from this node (will be recorded to TT and potentially become next "best_so_far" if strong enough)
  int legal_moves   = 0;
  int max_score     = -INF;

  CheckMasks masks = generate_check_masks(ctx);

  int scores[256];
  for (int i = 0; i < movelist.count; ++i) {
    scores[i] = score_move(ctx, movelist.moves[i], best_so_far, &masks);
  }

  for (int i = 0; i < movelist.count; ++i) {
    bump_highest_scored_move(i, &movelist, scores);

    Move move          = movelist.moves[i];
    PieceType attacker = get_piece_type(&ctx->bitboard_set, move_from(move));
    PieceType victim   = get_piece_type(&ctx->bitboard_set, move_to(move));
    bool is_quiet      = (move_type(move) == MOVE_NORMAL) && (victim == NO_PIECE);
    bool gives_check   = giving_check(move, attacker, &masks);

    MoveHistory history;
    make_move(ctx, movelist.moves[i], &history);

    if (king_in_check(&ctx->bitboard_set, !ctx->search_color)) {
      unmake_move(ctx, &history);
      continue;
    }

    legal_moves++;
 
    int score;

    int r = get_lmr_reduction(depth, legal_moves, is_quiet, gives_check, in_check);
    if (r > 0) {
      // reduced depth search with a zero-window (testing if it beats alpha)
      score = -negamax(ctx, depth-1-r, ply+1, -alpha-1, -alpha, ALLOW_NULL);
      
      // re-search: if the move is surprisingly good, search again at full depth and normal window
      if (score > alpha && score < beta)
        score = -negamax(ctx, depth-1, ply+1, -beta, -alpha, ALLOW_NULL);

    } else {
      // normal search same as before (no reduction)
      score = -negamax(ctx, depth-1, ply+1, -beta, -alpha, ALLOW_NULL);
    }

    unmake_move(ctx, &history);

    if (*ctx->should_stop) return 0;

    if (score > max_score) {
      max_score = score;
      best_move = movelist.moves[i]; // this will go to TT
    }

    if (score > alpha)
      alpha = score;

    if (alpha >= beta)
      break;
  }

  if (legal_moves == 0) {
    if (king_in_check(&ctx->bitboard_set, ctx->search_color))
      return -MATE_SCORE + ply;
    return 0;
  }

  TT_Flag flag;

  if (max_score <= original_alpha) {
    flag = TT_ALPHA;
  } else if (max_score >= beta) {
    flag = TT_BETA;
  } else {
    flag = TT_EXACT;
  }

  int tt_record_score = score_to_tt(max_score, ply);
  tt_record(ctx->hash_key, depth, tt_record_score, flag, best_move);

  return max_score;
}

// https://www.chessprogramming.org/Quiescence_Search
int quiescence_search(SF_Context *ctx, int ply, int alpha, int beta) {
  ctx->nodes++;

  if (check_time(ctx)) return 0;

  if (is_repetition(ctx)) return 0;

  if (ctx->history_count >= SF_MAX_HIST)
    return sf_evaluate_position(ctx); // avoid potential stack overflow (shouldn't happen)

  int original_alpha = alpha;
  int tt_score = 0;
  Move tt_move = 0;

  if (tt_probe(ctx->hash_key, 0, alpha, beta, &tt_score, &tt_move)) {
    return score_from_tt(tt_score, ply);
  }

  bool in_check = king_in_check(&ctx->bitboard_set, ctx->search_color);
  int max_score = -INF; // we'll track the best score to write to TT

  if (!in_check) {
    int stand_pat = sf_evaluate_position(ctx);

    if (stand_pat > max_score)
      max_score = stand_pat;

    if (stand_pat >= beta) {
      tt_record(ctx->hash_key, 0, stand_pat, TT_BETA, 0);
      return beta;
    }

    if (stand_pat > alpha)
      alpha = stand_pat; 
  }

  MoveList movelist;
  if (in_check) {
    movelist = generate_pseudo_legal_moves(ctx);
  } else {
    movelist = generate_noisy_moves(ctx);
  }

  Move best_so_far = tt_move;
  Move best_move   = 0;
  int legal_moves  = 0;

  CheckMasks masks = generate_check_masks(ctx);

  int scores[256];
  for (int i = 0; i < movelist.count; ++i) {
    scores[i] = score_move(ctx, movelist.moves[i], best_so_far, &masks);
  }

  for (int i = 0; i < movelist.count; ++i) {
    bump_highest_scored_move(i, &movelist, scores);

    MoveHistory history;
    make_move(ctx, movelist.moves[i], &history);

    if (king_in_check(&ctx->bitboard_set, !ctx->search_color)) {
      unmake_move(ctx, &history);
      continue;
    }

    legal_moves++;

    int score = -quiescence_search(ctx, ply+1, -beta, -alpha);

    unmake_move(ctx, &history);

    if (*ctx->should_stop) return 0;

    if (score > max_score) {
      max_score = score;
      best_move = movelist.moves[i];
    }

    if (score >= beta) {
      int tt_record_score = score_to_tt(score, ply);
      tt_record(ctx->hash_key, 0, tt_record_score, TT_BETA, best_move);
      return beta;
    }

    if (score > alpha) {
      alpha = score;
    } 
  }

  if (in_check && legal_moves==0) {
    return -MATE_SCORE + ply;
  }

  TT_Flag flag;

  if (max_score <= original_alpha) {
    flag = TT_ALPHA;
  } else {
    flag = TT_EXACT;
  }

  int tt_record_score = score_to_tt(max_score, ply);
  tt_record(ctx->hash_key, 0, tt_record_score, flag, best_move);

  return alpha;
}

int null_move_search(SF_Context *ctx, unsigned int depth, int ply, int beta) {
  int R = 2; // depth reduction
  
  MoveHistory null_history;
  make_null_move(ctx, &null_history);

  // zero-window search: only check out if score is greater than beta
  int null_score = -negamax(ctx, depth-1-R, ply+1, -beta, -beta+1, !ALLOW_NULL);

  unmake_null_move(ctx, &null_history);

  if (*ctx->should_stop) return 0;

  if (null_score >= beta) {
    return beta; // pruning succeeds
  }

  return -1;
}


static int score_move(const SF_Context *ctx, Move move, Move best_so_far, const CheckMasks *masks) {
  if (move == best_so_far)
    return INF;

  MoveType type = move_type(move);
  Square from   = move_from(move);
  Square to     = move_to(move);

  const BitboardSet *bbset = &ctx->bitboard_set;
  PieceType attacker = get_piece_type(bbset, from);
  PieceType victim   = get_piece_type(bbset, to);

  bool check      = giving_check(move, attacker, masks);
  bool capture    = victim != NO_PIECE;
  bool promote    = type == MOVE_PROMOTION;
  bool castle     = type == MOVE_CASTLING;
  bool en_passant = type == MOVE_EN_PASSANT;

  int score = 0;

  if (check)
    score += 50000;
  if (capture)
    score += 50000 + piece_value(victim)*10 - piece_value(attacker);
  if (promote)
    score += 60000;
  if (en_passant)
    score += 20000;
  if (castle)
    score += 10000;

  return score;
}

/* Move the highest scored move to the top of the move list */
static void bump_highest_scored_move(int i, MoveList *movelist, int *scores) {
  int best_i = i;
  
  for (int j = i+1; j < movelist->count; ++j)
    if (scores[j] > scores[best_i])
      best_i = j;

  if (best_i == i) return;

  Move tmp_m              = movelist->moves[i];
  movelist->moves[i]      = movelist->moves[best_i];
  movelist->moves[best_i] = tmp_m;
  
  int tmp_s      = scores[i];
  scores[i]      = scores[best_i];
  scores[best_i] = tmp_s;
}

static CheckMasks generate_check_masks(const SF_Context *ctx) {
  CheckMasks masks = {0, 0, 0, 0};
  
  Turn us   = ctx->search_color;
  Turn them = !us;
  const BitboardSet *bbset = &ctx->bitboard_set;
  
  if (bbset->kings[them] == 0) return masks; // safety

  Square enemy_king = GET_LSB(bbset->kings[them]);
  U64 occupied = bbset->occupied;

  masks.pawn   = pawn_attacks[them][enemy_king]; 
  masks.knight = knight_attacks[enemy_king];
  masks.bishop = get_bishop_attacks(enemy_king, occupied);
  masks.rook   = get_rook_attacks(enemy_king, occupied);

  return masks;
}


static inline bool check_time(SF_Context *ctx) {
  if (ctx->should_stop && *ctx->should_stop) return true;

  if ((ctx->nodes & 2047) == 0) { 
    if (SDL_GetTicks() - ctx->start_time >= ctx->time_limit) {
      if (ctx->should_stop) *ctx->should_stop = true;
      return true;
    }
  }

  return false;
}

static inline bool is_repetition(const SF_Context *ctx) {
  for (int i = ctx->history_count - 4; i >= 0; i -= 2)
    if (ctx->pos_history[i] == ctx->hash_key)
      return true; 
  return false;
}

/*
 * TT Mate Adjustments
 * When writing or reading a mate score into the transposition table,
 * We consider the score's distance (ply) from the root node.
 * This way we get quicker mates.
 */
static inline int score_to_tt(int score, int ply) {
  if (score > MATE_BOUND)  return score + ply;
  if (score < -MATE_BOUND) return score - ply;
  return score;
}
static inline int score_from_tt(int score, int ply) {
  if (score > MATE_BOUND)  return score - ply;
  if (score < -MATE_BOUND) return score + ply;
  return score;
}

static inline bool giving_check(Move move, PieceType attacker, const CheckMasks *masks) {
  Square to  = move_to(move);
  U64 to_bit = 1ULL << to;

  if (move_type(move) == MOVE_PROMOTION) {
    switch (move_promotion(move)) {
      case PROMOTE_BISHOP: return  (masks->bishop & to_bit) != 0;
      case PROMOTE_KNIGHT: return  (masks->knight & to_bit) != 0;
      case PROMOTE_ROOK:   return  (masks->rook   & to_bit) != 0;
      case PROMOTE_QUEEN:  return ((masks->bishop | masks->rook) & to_bit) != 0;
    }
  }

  switch (attacker) {
    case W_PAWN:   case B_PAWN:   return  (masks->pawn   & to_bit) != 0;
    case W_KNIGHT: case B_KNIGHT: return  (masks->knight & to_bit) != 0;
    case W_BISHOP: case B_BISHOP: return  (masks->bishop & to_bit) != 0;
    case W_ROOK:   case B_ROOK:   return  (masks->rook   & to_bit) != 0;
    case W_QUEEN:  case B_QUEEN:  return ((masks->bishop | masks->rook) & to_bit) != 0;
    default: return false;
  }
}

static inline bool has_non_pawn_material(const SF_Context *ctx) {
  Turn us = ctx->search_color;
  const BitboardSet *bbs = &ctx->bitboard_set;
  return (bbs->knights[us] | bbs->bishops[us] | bbs->rooks[us] | bbs->queens[us]) != 0;
}

/*
 * LMR (Late Move Reductions) Conditions:
 * 1. Depth must be at least 3 (do not reduce at shallow depths)
 * 2. Must have searched at least 3 moves already (protect TT move and good captures)
 * 3. The move must be quiet (no captures or promotions)
 * 4. The move must not give a check
 * 5. We must not currently be in check
 */
static inline int get_lmr_reduction(int depth, int legal_moves, bool is_quiet, bool gives_check, bool in_check) {
  if (depth >= 3 && legal_moves >= 4 && is_quiet && !gives_check && !in_check) {
    int reduction = 1;
    
    // aggressive reduction: if depth is high and the move is searched very late, reduce further
    if (depth >= 5 && legal_moves >= 6) {
      reduction = 2;
    }
    
    return reduction;
  }
  
  return 0; // LMR conditions aren't met so we'll keep going with a full-depth search
}

