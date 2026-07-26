#pragma once

#include "sockfish.h"

/* Internal Type for MoveHistory */
typedef struct {
  U64 hash;
  Square ep_sq;
  int mg_score[2];
  int eg_score[2];
  int game_phase;
  int halfmove_clock;
  int history_count;
  int history_head;
  uint8_t castling;
} Previous;

typedef struct {
  Previous prev;
  U64 overwritten_history_hash;
  Square captured_square;
  PieceType captured_piece;
  Move move;
} MoveHistory;

typedef struct {
  U64 hash;
  Square ep_sq;
  bool in_null_search;
} NullMoveHistory;

void make_move(SF_Context *ctx, Move move, MoveHistory *history);
void unmake_move(SF_Context *ctx, const MoveHistory *history);
void make_null_move(SF_Context *ctx, NullMoveHistory *history);
void unmake_null_move(SF_Context *ctx, const NullMoveHistory *history);
bool king_in_check(const BitboardSet *bbset, Turn color);
bool has_legal_en_passant_capture(const SF_Context *ctx);

PieceType get_piece_type(const BitboardSet *bbs, Square sq);
