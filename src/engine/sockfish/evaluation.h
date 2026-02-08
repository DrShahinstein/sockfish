#pragma once

#define PAWN_VALUE   100
#define KNIGHT_VALUE 300
#define BISHOP_VALUE 320
#define ROOK_VALUE   500
#define QUEEN_VALUE  950
#define KING_VALUE   2000

/* Tables... */
//static const int (piece?)_TABLE[64] = {
//
//};

int sf_evaluate_position(const SF_Context *ctx);
