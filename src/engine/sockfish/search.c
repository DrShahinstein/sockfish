#include "search.h"
#include "sockfish.h"
#include "evaluation.h"
#include "move_helper.h"
#include "movegen.h"
#include "transposition_table.h"
#include "thread.h"
#include <string.h>
#include <pthread.h>

static inline bool threefold_repetition(const SF_Context *ctx);
static inline bool fifty_move_draw(const SF_Context *ctx);
static inline bool giving_check(Move move, PieceType attacker, const CheckMasks *masks);
static inline int piece_value(PieceType p);
static inline bool has_non_pawn_material(const SF_Context *ctx);
static inline int get_lmr_reduction(int depth, int legal_moves, bool is_quiet, bool gives_check, bool in_check);
static inline void save_killer_move(SF_Context *ctx, Move move, int ply);
static inline void save_history_heuristic(SF_Context *ctx, Move m, int depth);

static void send_uci_info(const SF_Context *ctx, const HelperThreadData *thread_data, int max_score_so_far, int helper_count, int depth);

Move sf_search(const SF_Context *ctx) {
  SF_Context ctx_ = *ctx;
  ctx_.nodes      = 0;
  ctx_.start_time = get_time_ms();

  memset(ctx_.killer_moves,      0, sizeof(ctx_.killer_moves));
  memset(ctx_.history_heuristic, 0, sizeof(ctx_.history_heuristic));

  /* For Safety */
  bool local_stop = false;
  if (ctx_.should_stop == NULL)
    ctx_.should_stop = &local_stop;

  int num_threads = ctx_.threads;
  if (num_threads < 1) num_threads = 1;

  pthread_t *threads            = NULL;
  HelperThreadData *thread_data = NULL;
  int helper_count              = num_threads - 1;

  if (helper_count > 0) {
    threads     = (pthread_t*)malloc(helper_count * sizeof(pthread_t));
    thread_data = (HelperThreadData*)malloc(helper_count * sizeof(HelperThreadData));

    for (int i = 0; i < helper_count; ++i) {
      thread_data[i].ctx = ctx_;
      thread_data[i].ctx.should_stop = ctx_.should_stop;
      thread_data[i].thread_id = i + 1; // main-thread's ID=0, helper-threads=>1,2...
      pthread_create(&threads[i], NULL, helper_search_thread, &thread_data[i]);
    }
  }

  Move best_move = create_move(A1,A1);

  /* Main Thread Search Loop */
  for (int depth=1; depth <= MAX_DEPTH; ++depth) {
    if (is_depth_limit_exceeded(&ctx_, depth)) break;
    if (check_stop_conditions(&ctx_)) break;
    
    int alpha            = -INF;
    int beta             = +INF;
    int max_score_so_far = -INF;
    Move best_so_far     = best_move;

    MoveList movelist = generate_pseudo_legal_moves(&ctx_);
    if (movelist.count == 0) break;

    CheckMasks masks = generate_check_masks(ctx);

    int scores[256]; // scores[i] <===> movelist->moves[i]
    for (int i=0; i < movelist.count; ++i) {
      scores[i] = score_move(&ctx_, movelist.moves[i], best_so_far, &masks, ROOT_PLY);
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

    if (ctx_.allow_uci_info) {
      send_uci_info(&ctx_, thread_data, max_score_so_far, helper_count, depth);
    }
  }

  /* Shutdown Helper Threads */
  if (helper_count > 0) {
    *ctx_.should_stop = true;

    for (int i = 0; i < helper_count; ++i) {
      pthread_join(threads[i], NULL);
      ctx_.nodes += thread_data[i].ctx.nodes;
    }

    free(threads);
    free(thread_data);
  }

  ((SF_Context*)ctx)->nodes = ctx_.nodes;

  return best_move;
}

int negamax(SF_Context *ctx, int depth, int ply, int alpha, int beta, bool allow_null) {
  ctx->nodes++;

  if (ctx->history_count >= SF_MAX_HIST)
    return sf_evaluate_position(ctx); // avoid potential stack overflow (shouldn't happen)

  if (check_stop_conditions(ctx))
    return 0;

  if (threefold_repetition(ctx) || fifty_move_draw(ctx))
    return 0;

  if (depth == 0)
    return quiescence_search(ctx, ply, alpha, beta);

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
    scores[i] = score_move(ctx, movelist.moves[i], best_so_far, &masks, ply);
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

    /* First move: Make a full-window search */
    if (legal_moves == 1) {
      score = -negamax(ctx, depth-1, ply+1, -beta, -alpha, ALLOW_NULL);
    } 

    /* Next moves: Reductional approach */
    else {
      int r = get_lmr_reduction(depth, legal_moves, is_quiet, gives_check, in_check);
      
      /* Quick zero-window search at reduced depth */
      score = -negamax(ctx, depth-1-r, ply+1, -alpha-1, -alpha, ALLOW_NULL);

      /* If that search can surpass alpha, then the move is exceptionally good */
      if (score > alpha) {

        /* Run the same zero-window search without depth reduction */
        if (r > 0) {
          score = -negamax(ctx, depth-1, ply+1, -alpha-1, -alpha, ALLOW_NULL);
        }
        
        /* Still can surpass alpha? Stop being stubborn and make a full-window search */
        if (score > alpha && score < beta) {
          score = -negamax(ctx, depth-1, ply+1, -beta, -alpha, ALLOW_NULL);
        }
      }
    }

    unmake_move(ctx, &history);

    if (*ctx->should_stop) return 0;

    if (score > max_score) {
      max_score = score;
      best_move = movelist.moves[i]; // this will go to TT
    }

    if (score > alpha)
      alpha = score;

    if (alpha >= beta) {
      if (is_quiet) {
        if (ply < SF_MAX_PLY)
          save_killer_move(ctx, move, ply);
        save_history_heuristic(ctx, move, depth);
      }
      break;
    }
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

  if (check_stop_conditions(ctx))
    return 0;

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
    scores[i] = score_move(ctx, movelist.moves[i], best_so_far, &masks, ply);
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

int null_move_search(SF_Context *ctx, int depth, int ply, int beta) {
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

int score_move(const SF_Context *ctx, Move move, Move best_so_far, const CheckMasks *masks, int ply) {
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
  bool quiet      = type == MOVE_NORMAL && victim == NO_PIECE;

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

  if (ply < SF_MAX_PLY) {
    /* primary killer move */
    if (move == ctx->killer_moves[ply][0]) 
      score += 9000;

    /* secondary killer move */
    else if (move == ctx->killer_moves[ply][1]) 
      score += 8000;

    /* history heuristic */
    else if (quiet) {
      Turn c = ctx->search_color;
      int n  = ctx->history_heuristic[c][from][to];
      if (n > 1000) n=1000;
      score += n;
    }
  }

  return score;
}

/* Move the highest scored move to the top of the move list */
void bump_highest_scored_move(int i, MoveList *movelist, int *scores) {
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

CheckMasks generate_check_masks(const SF_Context *ctx) {
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

bool check_stop_conditions(SF_Context *ctx) {
  if (should_stop(ctx))
    return true;

  if ((ctx->nodes & 2047) == 0) { 
    if (ctx->nodes_limit > 0 && ctx->nodes >= ctx->nodes_limit) {
      if (ctx->should_stop) *ctx->should_stop = true;
      return true;
    }

    if (!ctx->infinite && ctx->time_limit > 0) {
      if (get_time_ms() - ctx->start_time >= ctx->time_limit) {
        if (ctx->should_stop) *ctx->should_stop = true;
        return true;
      }
    }
  }

  return false;
}

/*
 * TT Mate Adjustments
 * When writing or reading a mate score into the transposition table,
 * We consider the score's distance (ply) from the root node.
 * This way we get quicker mates.
 */
int score_to_tt(int score, int ply) {
  if (score > MATE_BOUND)  return score + ply;
  if (score < -MATE_BOUND) return score - ply;
  return score;
}
int score_from_tt(int score, int ply) {
  if (score > MATE_BOUND)  return score - ply;
  if (score < -MATE_BOUND) return score + ply;
  return score;
}

/* Helps converting score to UCI format (centipawn | mate-in-x) */
void format_score(int score, char *buf) {
  if (score > MATE_BOUND) {
    int moves_to_mate = (MATE_SCORE - score + 1) / 2;
    sprintf(buf, "mate %d", moves_to_mate);
  } else if (score < -MATE_BOUND) {
    int moves_to_mate = (-MATE_SCORE - score - 1) / 2;
    sprintf(buf, "mate %d", moves_to_mate);
  } else {
    sprintf(buf, "cp %d", score);
  }
}

/* Extracts principal variation (PV) by reading transposition table */
int extract_pv(const SF_Context *ctx, Move *pv_line, int max_len) {
  int count = 0;
  SF_Context temp_ctx = *ctx;

  while (count < max_len) {
    int tt_score;
    Move tt_move = 0;
    
    tt_probe(temp_ctx.hash_key, 0, -INF, INF, &tt_score, &tt_move);

    if (tt_move == 0)
      break;

    bool valid = false;
    MoveList list = generate_pseudo_legal_moves(&temp_ctx);
    for (int i = 0; i < list.count; ++i) {
      if (list.moves[i] == tt_move) {
        valid = true;
        break;
      }
    }
    
    if (!valid)
      break;

    pv_line[count++] = tt_move;

    MoveHistory hist;
    make_move(&temp_ctx, tt_move, &hist);
  }

  return count;
}






static inline bool threefold_repetition(const SF_Context *ctx) {
  int limit = ctx->history_count - ctx->halfmove_clock;
  if (limit < 0) limit = 0;

  for (int i = ctx->history_count - 4; i >= limit; i -= 2)
    if (ctx->pos_history[i] == ctx->hash_key)
      return true;
  return false;
}

static inline bool fifty_move_draw(const SF_Context *ctx) {
  return ctx->halfmove_clock >= 100;
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

/* Helps score_move() function */
static inline int piece_value(PieceType p) {
  switch (p) {
    case W_PAWN:   case B_PAWN:   return 100;
    case W_KNIGHT: case B_KNIGHT: return 320;
    case W_BISHOP: case B_BISHOP: return 330;
    case W_ROOK:   case B_ROOK:   return 500;
    case W_QUEEN:  case B_QUEEN:  return 900;
    case W_KING:   case B_KING:   return 20000;
    default: return -1;
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

static inline void save_killer_move(SF_Context *ctx, Move move, int ply) {
  bool new_primary_killer = (ctx->killer_moves[ply][0] != move);

  if (new_primary_killer) {
    ctx->killer_moves[ply][1] = ctx->killer_moves[ply][0];
    ctx->killer_moves[ply][0] = move;
  }
}

static inline void save_history_heuristic(SF_Context *ctx, Move m, int depth) {
  Turn c  = ctx->search_color;
  Move fr = move_from(m);
  Move to = move_to(m);
  ctx->history_heuristic[c][fr][to] += depth*depth;
}





static void send_uci_info(const SF_Context *ctx, const HelperThreadData *thread_data, int max_score_so_far, int helper_count, int depth) {
  U64 current_time = get_time_ms();
  U64 elapsed      = current_time - ctx->start_time;
  if (elapsed == 0) elapsed = 1;

  U64 total_nodes = ctx->nodes;
  if (helper_count > 0) {
    for (int i = 0; i < helper_count; ++i) {
      total_nodes += thread_data[i].ctx.nodes;
    }
  }

  U64 nps = (total_nodes * 1000) / elapsed;

  char score_str[32];
  format_score(max_score_so_far, score_str);

  Move pv_line[MAX_DEPTH];
  int pv_length = extract_pv(ctx, pv_line, depth);

  int hashfull = tt_get_hashfull();

  printf("info depth %d score %s time %llu nodes %llu nps %llu hashfull %d pv", depth, score_str,
      (unsigned long long) elapsed,
      (unsigned long long) total_nodes,
      (unsigned long long) nps,
      hashfull
  );

  for (int i=0; i < pv_length; ++i) {
    char move_buf[6];
    move_to_uci_string(pv_line[i], move_buf);
    printf(" %s", move_buf);
  }
  printf("\n");
  fflush(stdout);
}

