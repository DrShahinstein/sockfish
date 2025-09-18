#pragma once

#include "sockfish/sockfish.h"

typedef struct {
  Move move;
  PieceType captured_piece;
  Square captured_square;
  uint8_t prev_castling; // previous castling rights
  Square prev_ep_sq;
} MoveHistory;

void make_move(SF_Context *ctx, Move move, MoveHistory *history);
void unmake_move(SF_Context *ctx, const MoveHistory *history);
bool king_in_check(const BitboardSet *bbset, Turn color);

/*

MoveList sf_generate_moves(SF_Context *ctx); // movegen.h

=> 'move_helper.h' enables the definition of this function in 'movegen.c'

*/