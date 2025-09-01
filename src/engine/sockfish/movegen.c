#include "sockfish/movegen.h"
#include <stdlib.h>

MagicEntry rook_magic[64];
MagicEntry bishop_magic[64];

U64 pawn_attacks[2][64];
U64 knight_attacks[64];
U64 king_attacks[64];
U64 rook_magics[64]   = {0};
U64 bishop_magics[64] = {0};

static void attack_table_for_knight(void);
void init_attack_tables(void) {
  attack_table_for_knight();
}

void init_magic_bitboards(void) {
  return;
}

MoveList sf_generate_moves(const BitboardSet *bbset, Turn color) {
  MoveList movelist;
  movelist.count = 0;

  U64 occupancy = bbset->occupied;
  U64 friendly_pieces = bbset->all_pieces[color];
  U64 enemy_pieces = bbset->all_pieces[1-color];

  gen_pawns  (bbset->pawns  [color], &movelist, occupancy, friendly_pieces, enemy_pieces);
  gen_rooks  (bbset->rooks  [color], &movelist, occupancy, friendly_pieces);
  gen_knights(bbset->knights[color], &movelist, /*noneed*/ friendly_pieces);
  gen_bishops(bbset->bishops[color], &movelist, occupancy, friendly_pieces);
  gen_queens (bbset->queens [color], &movelist, occupancy, friendly_pieces);
  gen_kings  (bbset->kings  [color], &movelist, occupancy, friendly_pieces);

  return movelist;
}

void gen_pawns(Bitboard pawns, MoveList *movelist, U64 occupancy, U64 friendly_pieces, U64 enemy_pieces) {
  (void)pawns; (void)movelist; (void)occupancy; (void)friendly_pieces; (void)enemy_pieces;
}

void gen_rooks(Bitboard rooks,   MoveList *movelist, U64 occupancy, U64 friendly_pieces) {
  (void)rooks; (void)movelist; (void)occupancy; (void)friendly_pieces;
}

void gen_knights(Bitboard knights, MoveList *movelist, U64 friendly_pieces) {
  (void)knights; (void)movelist;

  U64 knights_copy = knights;

  while (knights_copy) {
    int knight_square = POP_LSB(&knights_copy);
    U64 attacks = knight_attacks[knight_square] & ~friendly_pieces;

    U64 attacks_copy = attacks;
    while (attacks_copy) {
      int target_square = POP_LSB(&attacks_copy);
      movelist->moves[movelist->count++] = (MoveSQ){knight_square, target_square};
    }
  }
}

void gen_bishops(Bitboard bishops, MoveList *movelist, U64 occupancy, U64 friendly_pieces) {
  (void)bishops; (void)movelist; (void)occupancy; (void)friendly_pieces;
}

void gen_queens(Bitboard queens, MoveList *movelist, U64 occupancy, U64 friendly_pieces) {
  (void)queens; (void)movelist; (void)occupancy; (void)friendly_pieces;
}

void gen_kings(Bitboard kings,   MoveList *movelist, U64 occupancy, U64 friendly_pieces) {
  (void)kings; (void)movelist; (void)occupancy; (void)friendly_pieces;
}

static void attack_table_for_knight(void) {
  int knight_moves[8][2] = {{1, 2},   {2, 1},   {2, -1}, {1, -2},
                            {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};

  for (int square = 0; square < 64; square++) {
    knight_attacks[square] = 0;
    int file = square % 8;
    int rank = square / 8;

    for (int i = 0; i < 8; i++) {
      int new_file = file + knight_moves[i][0];
      int new_rank = rank + knight_moves[i][1];

      if (new_file >= 0 && new_file < 8 && new_rank >= 0 && new_rank < 8) {
        int target_square = new_rank * 8 + new_file;
        SET_BIT(knight_attacks[square], target_square);
      }
    }
  }
}