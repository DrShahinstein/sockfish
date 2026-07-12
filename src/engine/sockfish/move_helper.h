#pragma once

#include "sockfish.h"

typedef struct {
  U64 prev_hash;
  Square prev_ep_sq;
  Square captured_square;
  PieceType captured_piece;
  Move move;
  int prev_mg_score[2];
  int prev_eg_score[2];
  int prev_game_phase;
  int prev_halfmove_clock;
  uint8_t prev_castling;
} MoveHistory;

void make_move(SF_Context *ctx, Move move, MoveHistory *history);
void unmake_move(SF_Context *ctx, const MoveHistory *history);
bool king_in_check(const BitboardSet *bbset, Turn color);

void make_null_move(SF_Context *ctx, MoveHistory *history);
void unmake_null_move(SF_Context *ctx, const MoveHistory *history);

PieceType get_piece_type(const BitboardSet *bbs, Square sq);

