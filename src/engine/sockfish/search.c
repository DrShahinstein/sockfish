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

static inline bool check_time(SF_Context *ctx);
static inline bool is_repetition(const SF_Context *ctx);
static inline int score_to_tt(int score, int ply);
static inline int score_from_tt(int score, int ply);

static int  score_move(const SF_Context *ctx, Move move, Move best_so_far);
static bool giving_check(const SF_Context *ctx, Move move);
static void bump_highest_scored_move(int i, MoveList *movelist, int *scores);

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

    int scores[256]; // scores[i] <===> movelist->moves[i]
    for (int i=0; i < movelist.count; ++i) {
      scores[i] = score_move(&ctx_, movelist.moves[i], best_so_far);
    }

    for (int i=0; i < movelist.count; ++i) {
      bump_highest_scored_move(i, &movelist, scores);

      MoveHistory history;
      make_move(&ctx_, movelist.moves[i], &history);

      if (king_in_check(&ctx_.bitboard_set, !ctx_.search_color)) {
        unmake_move(&ctx_, &history);
        continue;
      }

      int score = -negamax(&ctx_, depth-1, ROOT_PLY+1, -beta, -alpha);

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

int negamax(SF_Context *ctx, unsigned int depth, int ply, int alpha, int beta) {
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

  MoveList movelist = generate_pseudo_legal_moves(ctx);
  Move best_so_far  = tt_move; // the best result we obtained in previous nodes
  Move best_move    = 0;       // the best result from this node (will be recorded to TT and potentially become next "best_so_far" if strong enough)
  int legal_moves   = 0;
  int max_score     = -INF;

  int scores[256];
  for (int i = 0; i < movelist.count; ++i) {
    scores[i] = score_move(ctx, movelist.moves[i], best_so_far);
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

    int score = -negamax(ctx, depth-1, ply+1, -beta, -alpha);

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

  int scores[256];
  for (int i = 0; i < movelist.count; ++i) {
    scores[i] = score_move(ctx, movelist.moves[i], best_so_far);
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

/* Move the highest scored move to the top of the move list */
static void bump_highest_scored_move(int i, MoveList *movelist, int *scores) {
  int best_i = i;
  
  for (int j = i+1; j < movelist->count; ++j) {
    if (scores[j] > scores[best_i]) {
      best_i = j;
    }
  }

  if (best_i == i) return;

  Move tmp_m              = movelist->moves[i];
  movelist->moves[i]      = movelist->moves[best_i];
  movelist->moves[best_i] = tmp_m;
  
  int tmp_s      = scores[i];
  scores[i]      = scores[best_i];
  scores[best_i] = tmp_s;
}

static int score_move(const SF_Context *ctx, Move move, Move best_so_far) {
  if (move == best_so_far)
    return INF;

  MoveType type = move_type(move);
  Square from   = move_from(move);
  Square to     = move_to(move);

  const BitboardSet *bbset = &ctx->bitboard_set;
  PieceType attacker = get_piece_type(bbset, from);
  PieceType victim   = get_piece_type(bbset, to);

  bool check      = giving_check(ctx, move);
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

static bool giving_check(const SF_Context *ctx, Move move) {
  Square from   = move_from(move);
  Square to     = move_to(move);
  MoveType type = move_type(move);

  const BitboardSet *bbset = &ctx->bitboard_set;
  Turn us   = ctx->search_color;
  Turn them = !us;

  if (bbset->kings[them] == 0) return false; 

  Square enemy_king        = GET_LSB(bbset->kings[them]);
  PieceType checking_piece = get_piece_type(bbset, from);

  if (type == MOVE_PROMOTION) {
    PromotionType promo = move_promotion(move);

    if      (promo == PROMOTE_QUEEN)  checking_piece = (us == WHITE) ? W_QUEEN  : B_QUEEN;
    else if (promo == PROMOTE_ROOK)   checking_piece = (us == WHITE) ? W_ROOK   : B_ROOK;
    else if (promo == PROMOTE_BISHOP) checking_piece = (us == WHITE) ? W_BISHOP : B_BISHOP;
    else if (promo == PROMOTE_KNIGHT) checking_piece = (us == WHITE) ? W_KNIGHT : B_KNIGHT;
  }

  /* We'll make the move without worrying about legality (make-unmake) and see if it threatens the king. */

  // sliding pieces will need this updated occupancy map to find where they can go
  U64 new_occ = (bbset->occupied ^ (1ULL << from)) | (1ULL << to);

  if (type == MOVE_EN_PASSANT) {
    int ep_offset = (us == WHITE) ? -8 : +8;
    new_occ &= ~(1ULL << (to + ep_offset));
  }
 
  if (checking_piece == W_PAWN || checking_piece == B_PAWN)
    return (pawn_attacks[us][to] & (1ULL << enemy_king)) != 0;

  else if (checking_piece == W_KNIGHT || checking_piece == B_KNIGHT)
    return (knight_attacks[to] & (1ULL << enemy_king)) != 0;

  else if (checking_piece == W_BISHOP || checking_piece == B_BISHOP)
    return (get_bishop_attacks(to, new_occ) & (1ULL << enemy_king)) != 0;

  else if (checking_piece == W_ROOK || checking_piece == B_ROOK)
    return (get_rook_attacks(to, new_occ) & (1ULL << enemy_king)) != 0;

  else if (checking_piece == W_QUEEN || checking_piece == B_QUEEN)
    return ((get_bishop_attacks(to, new_occ) | get_rook_attacks(to, new_occ)) & (1ULL << enemy_king)) != 0;
 
  return false;
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

