#include "sockfish/search.h"
#include "sockfish/sockfish.h"
#include "sockfish/evaluation.h"
#include "sockfish/move_helper.h"
#include "sockfish/movegen.h"

#include <SDL3/SDL_timer.h>  /* SDL_GetTicks() */
#include <SDL3/SDL.h>        // DEBUG

#define INF 9999999
#define MATE_SCORE 9000000
#define MAX_DEPTH 40
#define SEARCH_TIME 2000  /* ms */
#define TIME_PASSED(t, nodes) ( (((nodes) & 2047) == 0) && (SDL_GetTicks() - (t) >= SEARCH_TIME) )

static int score_move(const SF_Context *ctx, Move move, Move best_so_far);
static void bump_highest_scored_move(int i, MoveList *movelist, int *scores);

Move sf_search(const SF_Context *ctx) {
  SF_Context ctx_ = *ctx;
  Move best_move  = create_move(A1,A1);
  U64 start_time  = SDL_GetTicks();

  for (int depth=1; depth <= MAX_DEPTH; ++depth) {
    if (should_stop(&ctx_) || TIME_PASSED(start_time, ctx_.nodes)) break;
    
    int alpha               = -INF;
    int beta                = +INF;
    int max_score_so_far    = -INF;
    Move best_so_far        = best_move;
    bool search_interrupted = false;

    MoveList movelist = generate_pseudo_legal_moves(&ctx_);
    if (movelist.count == 0) break;

    int scores[256]; // scores[i] <===> movelist->moves[i]
    for (int i=0; i < movelist.count; ++i) {
      scores[i] = score_move(&ctx_, movelist.moves[i], best_so_far);
    }

    for (int i=0; i < movelist.count; ++i) {
      if (should_stop(&ctx_) || TIME_PASSED(start_time, ctx_.nodes)) {
        search_interrupted = true;
        break;
      }

      /* move the highest scored move to the top of the move list */
      bump_highest_scored_move(i, &movelist, scores);

      MoveHistory history;
      make_move(&ctx_, movelist.moves[i], &history);

      if (king_in_check(&ctx_.bitboard_set, !ctx_.search_color)) {
        unmake_move(&ctx_, &history);
        continue;
      }

      int score = -negamax(&ctx_, depth-1, -beta, -alpha);

      unmake_move(&ctx_, &history);

      if (score > max_score_so_far) {
        max_score_so_far = score;
        best_so_far      = movelist.moves[i];
      }

      if (score > alpha) {
        alpha = score;
      }
    }

    if (search_interrupted) break;

    best_move = best_so_far;
  }

  return best_move;
}

int negamax(SF_Context *ctx, unsigned int depth, int alpha, int beta) {
  ctx->nodes++;

  (void)depth; (void)alpha; (void)beta;
  return 0;
}

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

  bool capture    = victim != NO_PIECE;
  bool en_passant = type == MOVE_EN_PASSANT;
  bool promote    = type == MOVE_PROMOTION;
  bool castle     = type == MOVE_CASTLING;

  if (capture && promote) {
    return 100000;
  }

  if (capture) {
    return 10000 + piece_value(victim)*10 - piece_value(attacker);
  }

  if (en_passant) {
    return 10000;
  }

  if (promote) {
    return 5000;
  }

  if (castle) {
    return 4000;
  }

  // TODO: king_in_check() issue

  return 0;
}

