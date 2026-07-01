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

    // TODO: order_moves(&ctx_, &movelist, best_so_far);

    for (int i=0; i < movelist.count; ++i) {
      if (should_stop(&ctx_) || TIME_PASSED(start_time, ctx_.nodes)) {
        search_interrupted = true;
        break;
      }

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

