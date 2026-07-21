#pragma once

#include "sockfish.h"
#include "movegen.h"

#define INF 9999999
#define MATE_SCORE 9000000
#define MATE_BOUND 8000000
#define MAX_DEPTH 40

static const int ROOT_PLY=0;
static const int ALLOW_NULL=true;
static const int HH_LIMIT=8192;
static const int HH_DELTA_MAX=1024;

typedef struct {
  U64 pawn;
  U64 knight;
  U64 bishop;
  U64 rook;
} CheckMasks;

Move sf_search(const SF_Context *ctx);
int negamax(SF_Context *ctx, int depth, int ply, int alpha, int beta, bool allow_null);
int quiescence_search(SF_Context *ctx, int ply, int alpha, int beta);
int null_move_search(SF_Context *ctx, int depth, int ply, int beta);
int score_move(const SF_Context *ctx, Move move, Move best_so_far, const CheckMasks *masks, int ply);

bool check_stop_conditions(SF_Context *ctx);
void bump_highest_scored_move(int i, MoveList *movelist, int *scores);
CheckMasks generate_check_masks(const SF_Context *ctx);
int extract_pv(const SF_Context *ctx, Move *pv_line, int max_len);
int score_to_tt(int score, int ply);
int score_from_tt(int score, int ply);
void format_score(int score, char *buf);

static inline int clamp_int(int value, int min, int max) {
  if (value < min) return min;
  if (value > max) return max;
  return value;
}

