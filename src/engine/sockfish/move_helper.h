#pragma once

#include "sockfish/sockfish.h"

typedef struct {
  U64 prev_hash;
  Square prev_ep_sq;
  Square captured_square;
  PieceType captured_piece;
  Move move;
  uint8_t prev_castling;
} MoveHistory;

void make_move(SF_Context *ctx, Move move, MoveHistory *history);
void unmake_move(SF_Context *ctx, const MoveHistory *history);
bool king_in_check(const BitboardSet *bbset, Turn color);

PieceType get_piece_type(const BitboardSet *bbs, Square sq);

/*

MoveList sf_generate_moves(SF_Context *ctx); // movegen.h

=> 'move_helper.h' enables the definition of this function in 'movegen.c'

*/