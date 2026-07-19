#include "thread.h"
#include "movegen.h"
#include "move_helper.h"
#include "evaluation.h"
#include "transposition_table.h"
#include "search.h"

void *helper_search_thread(void *arg) {
  HelperThreadData *data = (HelperThreadData*)arg;
  SF_Context ctx_        = data->ctx;
  int thread_id          = data->thread_id;

  ctx_.nodes = 0;
  ctx_.start_time = get_time_ms();

  int depth_offset = (thread_id + 1) / 2; // Lazy SMP asymmetry

  Move best_move = create_move(A1,A1);

  for (int depth=1; depth <= MAX_DEPTH; ++depth) {
    if (is_depth_limit_exceeded(&ctx_, depth)) break;
    if (check_stop_conditions(&ctx_)) break;

    int search_depth = depth + depth_offset;
    if (search_depth > MAX_DEPTH) search_depth = MAX_DEPTH;

    int alpha            = -INF;
    int beta             = +INF;
    int max_score_so_far = -INF;
    Move best_so_far     = best_move;

    MoveList movelist = generate_pseudo_legal_moves(&ctx_);
    if (movelist.count == 0) break;

    CheckMasks masks = generate_check_masks(&ctx_);

    int scores[256];
    for (int i = 0; i < movelist.count; ++i) {
      scores[i] = score_move(&ctx_, movelist.moves[i], best_so_far, &masks, ROOT_PLY);
    }

    for (int i = 0; i < movelist.count; ++i) {
      bump_highest_scored_move(i, &movelist, scores);

      MoveHistory history;
      make_move(&ctx_, movelist.moves[i], &history);

      if (king_in_check(&ctx_.bitboard_set, !ctx_.search_color)) {
        unmake_move(&ctx_, &history);
        continue;
      }

      int score = -negamax(&ctx_, search_depth-1, ROOT_PLY+1, -beta, -alpha, ALLOW_NULL);

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
    tt_record(ctx_.hash_key, search_depth, tt_record_score, TT_EXACT, best_so_far);

    best_move = best_so_far;
  }

  data->ctx.nodes = ctx_.nodes;
  
  return NULL;
}

