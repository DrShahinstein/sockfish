#include "search.h"
#include "sockfish.h"
#include "evaluation.h"
#include "move_helper.h"
#include "movegen.h"
#include "transposition_table.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

static inline bool threefold_repetition(const SF_Context *ctx);
static bool try_tt_cutoff(const SF_Context *ctx, int depth, int ply, int alpha, int beta, int *score, Move *best_move);
static inline bool fifty_move_draw(SF_Context *ctx, int ply, int *draw_or_mate);
static inline bool likely_giving_check(Move move, PieceType attacker, const CheckMasks *masks);
static inline int piece_value(PieceType p);
static inline bool has_non_pawn_material(const SF_Context *ctx);
static inline int get_null_move_reduction(int depth, int static_eval, int beta);
static inline int get_lmr_reduction(int depth, int legal_moves, bool is_quiet, bool gives_check, bool in_check);
static inline void save_killer_move(SF_Context *ctx, Move move, int ply);
static inline void update_history_heuristic(SF_Context *ctx, Move cutoff_move, const Move *failed_quiet_moves, int failed_quiet_count, int depth);
static inline void adjust_hh_entry(SF_Context *ctx, Move move, int delta);

static bool movelist_has_legal_move(SF_Context *ctx, const MoveList *movelist);
static bool position_has_legal_move(SF_Context *ctx);
static bool stand_pat_is_legal(SF_Context *ctx, const MoveList *noisy_moves);
static void send_uci_info(const SF_Context *ctx, const HelperThreadData *thread_data, int max_score_so_far, int helper_count, int depth);

Move sf_search(const SF_Context *ctx) {
  SF_Context ctx_ = *ctx;
  ctx_.nodes       = 0;
  ctx_.start_time  = get_time_ms();
  ctx_.nmp_min_ply = 0;

  memset(ctx_.killer_moves,      0, sizeof(ctx_.killer_moves));
  memset(ctx_.history_heuristic, 0, sizeof(ctx_.history_heuristic));

  /* For Safety */
  atomic_bool local_stop = false;
  if (ctx_.should_stop == NULL)
    ctx_.should_stop = &local_stop;

  MoveList root_moves = sf_generate_moves(&ctx_);
  if (root_moves.count == 0) {
    ((SF_Context*)ctx)->nodes = 0;
    return MOVE_NONE;
  }

  Move best_move = root_moves.moves[0];

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
      atomic_init(&thread_data[i].nodes, 0);
      pthread_create(&threads[i], NULL, helper_search_thread, &thread_data[i]);
    }
  }

  /* Main Thread Search Loop */
  for (int depth=1; depth <= MAX_DEPTH; ++depth) {
    if (is_depth_limit_exceeded(&ctx_, depth)) break;
    if (check_stop_conditions(&ctx_))          break;
    
    int alpha            = -INF;
    int beta             = +INF;
    int max_score_so_far = -INF;
    Move best_so_far     = best_move;

    MoveList movelist = root_moves;
    CheckMasks masks  = generate_check_masks(&ctx_);

    int scores[MOVELIST_CAPACITY]; // scores[i] <===> movelist->moves[i]
    for (int i=0; i < movelist.count; ++i) {
      scores[i] = score_move(&ctx_, movelist.moves[i], best_so_far, &masks, ROOT_PLY);
    }

    for (int i=0; i < movelist.count; ++i) {
      bump_highest_scored_move(i, &movelist, scores);

      MoveHistory history;
      make_move(&ctx_, movelist.moves[i], &history);

      int score = -negamax(&ctx_, depth-1, ROOT_PLY+1, -beta, -alpha, ALLOW_NULL);

      unmake_move(&ctx_, &history);

      if (should_stop(&ctx_)) break;

      if (score > max_score_so_far) {
        max_score_so_far = score;
        best_so_far      = movelist.moves[i];
      }

      if (score > alpha) {
        alpha = score;
      }
    }

    if (should_stop(&ctx_)) break;

    int tt_record_score = score_to_tt(max_score_so_far, ROOT_PLY);
    tt_record(sf_tt_hash_key(&ctx_), depth, tt_record_score, TT_EXACT, best_so_far);

    best_move = best_so_far;

    if (ctx_.allow_uci_info) {
      send_uci_info(&ctx_, thread_data, max_score_so_far, helper_count, depth);
    }
  }

  /* Shutdown Helper Threads */
  if (helper_count > 0) {
    request_search_stop(&ctx_);

    for (int i = 0; i < helper_count; ++i) {
      pthread_join(threads[i], NULL);
      ctx_.nodes += atomic_load_explicit(&thread_data[i].nodes, memory_order_relaxed);
    }

    free(threads);
    free(thread_data);
  }

  ((SF_Context*)ctx)->nodes = ctx_.nodes;

  return best_move;
}

int negamax(SF_Context *ctx, int depth, int ply, int alpha, int beta, bool allow_null) {
  ctx->nodes++;

  if (ply >= SF_MAX_PLY)
    return sf_evaluate_position(ctx); // avoid potential stack overflow (shouldn't happen)

  if (check_stop_conditions(ctx))
    return 0;

  if (threefold_repetition(ctx))
    return 0;

  int draw_or_mate;
  if (fifty_move_draw(ctx, ply, &draw_or_mate))
    return draw_or_mate;

  if (depth <= 0)
    return quiescence_search(ctx, ply, alpha, beta);

  int original_alpha = alpha;
  int tt_score = 0;
  Move tt_move = 0;

  if (try_tt_cutoff(ctx, depth, ply, alpha, beta, &tt_score, &tt_move))
    return tt_score;

  int static_eval = sf_evaluate_position(ctx);
  bool in_check   = king_in_check(&ctx->bitboard_set, ctx->search_color);
  bool nmp        = allow_null && ply >= ctx->nmp_min_ply &&
                    depth >= 3 && !in_check &&
                    beta > -MATE_BOUND && beta < MATE_BOUND &&
                    has_non_pawn_material(ctx) && static_eval >= beta;

  if (nmp) {
    if (null_move_search(ctx, depth, ply, beta, static_eval))
      return beta;

    if (should_stop(ctx))
      return 0;
  }

  MoveList movelist = generate_pseudo_legal_moves(ctx);
  Move best_so_far  = tt_move; // the best result we obtained in previous nodes
  Move best_move    = 0;       // the best result from this node (will be recorded to TT and potentially become next "best_so_far" if strong enough)
  int legal_moves   = 0;
  int max_score     = -INF;

  CheckMasks masks = generate_check_masks(ctx);

  int scores[MOVELIST_CAPACITY];
  for (int i=0; i < movelist.count; ++i) {
    scores[i] = score_move(ctx, movelist.moves[i], best_so_far, &masks, ply);
  }

  Move quiets[MOVELIST_CAPACITY];
  int quiets_count=0;

  for (int i = 0; i < movelist.count; ++i) {
    bump_highest_scored_move(i, &movelist, scores);

    Move move          = movelist.moves[i];
    PieceType attacker = get_piece_type(&ctx->bitboard_set, move_from(move));
    PieceType victim   = get_piece_type(&ctx->bitboard_set, move_to(move));
    bool is_quiet      = (move_type(move) == MOVE_NORMAL) && (victim == NO_PIECE);
    bool gives_check   = likely_giving_check(move, attacker, &masks);

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

    if (should_stop(ctx)) return 0;

    if (score > max_score) {
      max_score = score;
      best_move = movelist.moves[i]; // this will go to TT
    }

    if (score > alpha)
      alpha = score;

    if (alpha >= beta) {
      if (is_quiet) {
        if (ply < SF_MAX_PLY) {
          save_killer_move(ctx, move, ply);
        }
        update_history_heuristic(ctx, move, quiets, quiets_count, depth);
      }
      break;
    }

    if (is_quiet && quiets_count < MOVELIST_CAPACITY)
      quiets[quiets_count++] = move;
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
  tt_record(sf_tt_hash_key(ctx), depth, tt_record_score, flag, best_move);

  return max_score;
}

// https://www.chessprogramming.org/Quiescence_Search
int quiescence_search(SF_Context *ctx, int ply, int alpha, int beta) {
  ctx->nodes++;

  if (check_stop_conditions(ctx))
    return 0;

  if (ply >= SF_MAX_PLY)
    return sf_evaluate_position(ctx); // avoid potential stack overflow (shouldn't happen)

  if (threefold_repetition(ctx))
    return 0;

  int draw_or_mate;
  if (fifty_move_draw(ctx, ply, &draw_or_mate))
    return draw_or_mate;

  int original_alpha = alpha;
  int tt_score = 0;
  Move tt_move = 0;

  if (try_tt_cutoff(ctx, 0, ply, alpha, beta, &tt_score, &tt_move))
    return tt_score;

  bool in_check = king_in_check(&ctx->bitboard_set, ctx->search_color);
  int max_score = -INF; // we'll track the best score to write to TT

  MoveList movelist = in_check ? generate_pseudo_legal_moves(ctx) : generate_noisy_moves(ctx);

  if (!in_check) {
    int stand_pat = sf_evaluate_position(ctx);

    if (stand_pat > max_score)
      max_score = stand_pat;

    if (stand_pat >= beta) {
      if (!stand_pat_is_legal(ctx, &movelist))
        return 0;

      tt_record(sf_tt_hash_key(ctx), 0, stand_pat, TT_BETA, 0);
      return beta;
    }

    if (stand_pat > alpha)
      alpha = stand_pat; 
  }

  Move best_so_far = tt_move;
  Move best_move   = 0;
  int legal_moves  = 0;

  CheckMasks masks = generate_check_masks(ctx);

  int scores[MOVELIST_CAPACITY];
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

    if (should_stop(ctx)) return 0;

    if (score > max_score) {
      max_score = score;
      best_move = movelist.moves[i];
    }

    if (score >= beta) {
      int tt_record_score = score_to_tt(score, ply);
      tt_record(sf_tt_hash_key(ctx), 0, tt_record_score, TT_BETA, best_move);
      return beta;
    }

    if (score > alpha) {
      alpha = score;
    } 
  }

  if (legal_moves == 0) {
    /* Checkmate */
    if (in_check)
      return -MATE_SCORE + ply;

    /* Stalemate */
    if (!position_has_legal_move(ctx))
      return 0;
  }

  TT_Flag flag;

  if (max_score <= original_alpha) {
    flag = TT_ALPHA;
  } else {
    flag = TT_EXACT;
  }

  int tt_record_score = score_to_tt(max_score, ply);
  tt_record(sf_tt_hash_key(ctx), 0, tt_record_score, flag, best_move);

  return alpha;
}

bool null_move_search(SF_Context *ctx, int depth, int ply, int beta, int static_eval) {
  int reduction = get_null_move_reduction(depth, static_eval, beta);
  
  MoveHistory null_history;
  make_null_move(ctx, &null_history);

  /* A null move only needs a zero-window search to prove a beta cutoff. */
  int null_score = -negamax(ctx, depth-1-reduction, ply+1, -beta, -beta+1, false);

  unmake_null_move(ctx, &null_history);

  if (should_stop(ctx) || null_score < beta)
    return false;

  /*
   * At shallow depths the null result is sufficiently cheap and reliable.
   * At deeper nodes, verify the cutoff from the real position with null-move
   * pruning disabled at this node. This protects zugzwang-like positions
   * where passing can look better than every legal move.
   */
  if (depth < 10 || ctx->nmp_min_ply != 0)
    return true;

  int verification_depth   = depth - reduction;
  int previous_nmp_min_ply = ctx->nmp_min_ply;
  int verification_span    = clamp_int((3 * verification_depth) / 4, 1, verification_depth);
  ctx->nmp_min_ply = ply + verification_span;

  int verification_score = negamax(ctx, verification_depth, ply, beta-1, beta, false);
  ctx->nmp_min_ply = previous_nmp_min_ply;

  return !should_stop(ctx) && verification_score >= beta;
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

  bool check      = likely_giving_check(move, attacker, masks);
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

  bool killer_scored=false;

  if (ply < SF_MAX_PLY) {
    /* primary killer move */
    if (move == ctx->killer_moves[ply][0]) {
      score += 10000;
      killer_scored=true;
    }

    /* secondary killer move */
    else if (move == ctx->killer_moves[ply][1]) {
      score += 9000;
      killer_scored=true;
    }
  }

  /* history heuristic with quiet moves */
  if (quiet && !killer_scored) {
    Turn c = ctx->search_color;
    score += ctx->history_heuristic[c][from][to];
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
      request_search_stop(ctx);
      return true;
    }

    if (!ctx->infinite && ctx->time_limit > 0) {
      if (get_time_ms() - ctx->start_time >= ctx->time_limit) {
        request_search_stop(ctx);
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
    TT_Data tt_data;
    if (!tt_probe(sf_tt_hash_key(&temp_ctx), &tt_data) || tt_data.best_move == MOVE_NONE) {
      break;
    }

    Move tt_move = tt_data.best_move;

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






static bool try_tt_cutoff(const SF_Context *ctx, int depth, int ply, int alpha, int beta, int *score, Move *best_move) {
  TT_Data data;
  if (!tt_probe(sf_tt_hash_key(ctx), &data))
    return false;

  *best_move = data.best_move;
  if (data.depth < depth)
    return false;

  int decoded_score = score_from_tt(data.score, ply);
  bool cutoff =  data.flag == TT_EXACT                            ||
                (data.flag == TT_ALPHA && decoded_score <= alpha) ||
                (data.flag == TT_BETA  && decoded_score >= beta);

  if (!cutoff)
    return false;

  *score = decoded_score;

  return true;
}

static inline bool threefold_repetition(const SF_Context *ctx) {
  if (ctx->history_count <= 0 || ctx->pos_history[ctx->history_head] != ctx->hash_key || ctx->in_null_search)
    return false;

  int reversible_plies = ctx->halfmove_clock;
  int available_plies  = ctx->history_count - 1;

  if (reversible_plies > available_plies)
    reversible_plies = available_plies;

  int repetitions = 0;
  for (int distance = 0; distance <= reversible_plies; distance += 2) {
    int history_index = ctx->history_head - distance;
    if (history_index < 0)
      history_index += SF_MAX_HIST;

    if (ctx->pos_history[history_index] == ctx->hash_key && ++repetitions >= 3)
      return true;
  }

  return false;
}

static inline bool fifty_move_draw(SF_Context *ctx, int ply, int *draw_or_mate) {
  if (ctx->in_null_search ||
      ctx->halfmove_clock < FIFTY_MOVE_RULE_PLY_LIMIT) {
    return false;
  }

  /* Checkmate overrides the fifty-move rule */
  bool in_check = king_in_check(&ctx->bitboard_set, ctx->search_color);
  if (in_check && !position_has_legal_move(ctx))
    *draw_or_mate = -MATE_SCORE + ply;
  else
    *draw_or_mate = 0;

  return true;
}

/* Fast direct-check detection. Intentionally ignores discovered checks for performance. */
static inline bool likely_giving_check(Move move, PieceType attacker, const CheckMasks *masks) {
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

/* Returns as soon as one move in an already-generated candidate list is legal. */
static bool movelist_has_legal_move(SF_Context *ctx, const MoveList *movelist) {
  for (int i = 0; i < movelist->count; ++i) {
    MoveHistory history;
    make_move(ctx, movelist->moves[i], &history);
    bool legal = !king_in_check(&ctx->bitboard_set, !ctx->search_color);
    unmake_move(ctx, &history);

    if (legal)
      return true;
  }

  return false;
}

/* Generates every pseudo-legal move when a subset cannot prove non-terminal state. */
static bool position_has_legal_move(SF_Context *ctx) {
  MoveList movelist = generate_pseudo_legal_moves(ctx);
  return movelist_has_legal_move(ctx, &movelist);
}

static bool stand_pat_is_legal(SF_Context *ctx, const MoveList *noisy_moves) {
  return movelist_has_legal_move(ctx, noisy_moves) || position_has_legal_move(ctx);
}

static inline int get_null_move_reduction(int depth, int static_eval, int beta) {
  int reduction = 2;

  /* Search deeper positions more aggressively. */
  reduction += depth / 6;

  /* A comfortably fail-high static evaluation makes a larger reduction safer. */
  int eval_margin = static_eval - beta;
  reduction += clamp_int(eval_margin / 200, 0, 2);

  return clamp_int(reduction, 2, depth - 1);
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

/*
 * Adjust History Heuristic Entry
 *
 * Updates a move's history score safely.
 * Positive delta rewards the move, negative penalizes it.
 *
 * Uses a "gravity" effect so updates shrink as the score nears HH_LIMIT.
 * - HH_LIMIT: The absolute ceiling/floor for a move's score (+/-).
 * - HH_DELTA_MAX: The biggest bonus or penalty allowed in a single step.
 */
static inline void adjust_hh_entry(SF_Context *ctx, Move move, int delta) {
  Turn c      = ctx->search_color;
  Square from = move_from(move);
  Square to   = move_to(move);
  int *entry  = &ctx->history_heuristic[c][from][to];

  delta = clamp_int(delta, -HH_DELTA_MAX, HH_DELTA_MAX);

  *entry += delta - (*entry * abs(delta)) / HH_LIMIT;
  *entry = clamp_int(*entry, -HH_LIMIT, HH_LIMIT);
}

/*
 * Called when a quiet move successfully causes a beta-cutoff.
 * Rewards the winning move based on depth (capped at HH_DELTA_MAX),
 * and penalizes all the quiet moves we tried before it that failed.
 */
static inline void update_history_heuristic(SF_Context *ctx, Move cutoff_move, const Move *failed_quiet_moves, int failed_quiet_count, int depth) {
  int bonus = clamp_int(depth*depth, 0, HH_DELTA_MAX);
  adjust_hh_entry(ctx, cutoff_move, bonus);

  int malus = bonus / 2;
  for (int i = 0; i < failed_quiet_count; ++i) {
    adjust_hh_entry(ctx, failed_quiet_moves[i], -malus);
  }
}









static void send_uci_info(const SF_Context *ctx, const HelperThreadData *thread_data, int max_score_so_far, int helper_count, int depth) {
  U64 current_time = get_time_ms();
  U64 elapsed      = current_time - ctx->start_time;
  if (elapsed == 0) elapsed = 1;

  U64 total_nodes = ctx->nodes;
  if (helper_count > 0) {
    for (int i = 0; i < helper_count; ++i) {
      total_nodes += atomic_load_explicit(&thread_data[i].nodes, memory_order_relaxed);
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
