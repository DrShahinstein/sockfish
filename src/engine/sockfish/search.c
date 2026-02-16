#include "sockfish/search.h"
#include "sockfish/evaluation.h"
#include "sockfish/movegen.h"
#include "sockfish/move_helper.h"
#include "sockfish/sockfish.h"

#include <SDL3/SDL.h> // DEBUG

#define INF 50000
#define MATE_SCORE 49000
#define DEPTH 3

static int negamax(SF_Context *ctx, int depth, int alpha, int beta);

Move sf_search(const SF_Context *ctx) {
  SF_Context ctx_ = *ctx;
  MoveList moves  = sf_generate_moves(&ctx_);

  if (moves.count == 0) {
    SDL_Log("No legal moves available"); // DEBUG
    return create_move(0, 0);
  }

  Move best     = moves.moves[0];
  int max_score = -INF;

  SDL_Log("Searching %d moves at depth %d...", moves.count, DEPTH); // DEBUG

  for (int i = 0; i < moves.count; ++i) {
    if (should_stop(&ctx_)) break;

    MoveHistory history;
    make_move(&ctx_, moves.moves[i], &history);

    int score = -negamax(&ctx_, DEPTH, -INF, +INF);

    unmake_move(&ctx_, &history);

    if (score > max_score) {
      max_score = score;
      best      = moves.moves[i];

      /* --Debug-- */
      char from_alg[3], to_alg[3];
      sq_to_alg(move_from(best), from_alg);
      sq_to_alg(move_to(best), to_alg);
      SDL_Log("New best: %s%s (score: %d)", from_alg, to_alg, max_score);
      /* end */
    }
  }

  SDL_Log("Final best move score: %d", max_score); // DEBUG

  return best;
}

static int negamax(SF_Context *ctx, int depth, int alpha, int beta) {
  Turn t = ctx->search_color;

  if (depth == 0) {
    int eval = sf_evaluate_position(ctx);
    return t==WHITE ? eval : -eval;
  }

  MoveList moves = sf_generate_moves(ctx);

  if (moves.count == 0) {
    if (king_in_check(&ctx->bitboard_set, t))
      return -MATE_SCORE + (DEPTH - depth);
    return 0;
  }

  int max_score = -INF;
  for (int i = 0; i < moves.count; ++i) {
    if (should_stop(ctx)) return 0;

    MoveHistory history;
    make_move(ctx, moves.moves[i], &history);

    int score = -negamax(ctx, depth-1, -beta, -alpha);

    unmake_move(ctx, &history);

    if (score > max_score) {
      max_score = score;
    }

    if (score > alpha) {
      alpha = score;
    }

    if (alpha >= beta) {
      break;
    }
  }

  return max_score;
}
