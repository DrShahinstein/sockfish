#pragma once

#include "sockfish/sockfish.h"

typedef struct {
  MoveSQ moves[256]; // max possible moves in a chess position is =218
  int count;
} MoveList;

void gen_pawns  (Bitboard pawns,   MoveList *movelist);
void gen_rooks  (Bitboard rooks,   MoveList *movelist);
void gen_knights(Bitboard knights, MoveList *movelist);
void gen_bishops(Bitboard bishops, MoveList *movelist);
void gen_queens (Bitboard queens,  MoveList *movelist);
void gen_kings  (Bitboard kings,   MoveList *movelist);
MoveList sf_generate_moves(const BitboardSet *bbset, Turn color);
// MoveList sf_generate_legal_moves();