#include "sockfish/movegen.h"

MoveList sf_generate_moves(const BitboardSet *bbset, Turn color) {
  MoveList movelist;
  movelist.count = 0;

  gen_pawns  (bbset->pawns  [color], &movelist);
  gen_rooks  (bbset->rooks  [color], &movelist);
  gen_knights(bbset->knights[color], &movelist);
  gen_bishops(bbset->bishops[color], &movelist);
  gen_queens (bbset->queens [color], &movelist);
  gen_kings  (bbset->kings  [color], &movelist);

  return movelist;
}

void gen_pawns(Bitboard pawns, MoveList *movelist) {
  (void)pawns;

  for (int i = 0; i < 8; ++i) {
    movelist->moves[movelist->count++] = (Move){.fr = 1, .fc = 1, .tr = 2, .tc = 1};
  }
}

void gen_rooks(Bitboard rooks, MoveList *movelist) {
  (void)rooks; (void)movelist;
}

void gen_knights(Bitboard knights, MoveList *movelist) {
  (void)knights; (void)movelist;
}

void gen_bishops(Bitboard bishops, MoveList *movelist) {
  (void)bishops; (void)movelist;
}

void gen_queens(Bitboard queens, MoveList *movelist) {
  (void)queens; (void)movelist;
}

void gen_kings(Bitboard kings, MoveList *movelist) {
  (void)kings; (void)movelist;
}