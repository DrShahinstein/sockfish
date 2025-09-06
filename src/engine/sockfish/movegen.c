#include "sockfish/movegen.h"
#include <stdlib.h>

MagicEntry rook_magic[64];
MagicEntry bishop_magic[64];

U64 pawn_attacks[2][64];
U64 knight_attacks[64];
U64 king_attacks[64];
U64 rook_magics[64]   = {0};
U64 bishop_magics[64] = {0};

static void attack_table_for_pawn(void);
static void attack_table_for_knight(void);
static void attack_table_for_king(void);
void init_attack_tables(void) {
  attack_table_for_pawn();
  attack_table_for_knight();
  attack_table_for_king();
}

void init_magic_bitboards(void) {
  return;
}

MoveList sf_generate_moves(const BitboardSet *bbset, Turn color, uint8_t castling_rights) {
  MoveList movelist;
  movelist.count = 0;

  gen_pawns  (bbset, &movelist, color);
  gen_rooks  (bbset, &movelist, color);
  gen_knights(bbset, &movelist, color);
  gen_bishops(bbset, &movelist, color);
  gen_queens (bbset, &movelist, color);
  gen_kings  (bbset, &movelist, color, castling_rights);

  return movelist;
}

// incomplete: en-passant, promotion
void gen_pawns(const BitboardSet *bbset, MoveList *movelist, Turn color) {
  U64 pawns        = bbset->pawns[color];
  U64 enemy_pieces = bbset->all_pieces[!color];
  U64 occupancy    = bbset->occupied;
  int direction    = (color == WHITE) ? 1 : -1;

  while (pawns) {
    int pawn_square = POP_LSB(&pawns);
    int rank        = pawn_square / 8;

    U64 attacks        = pawn_attacks[color][pawn_square] & enemy_pieces;
    U64 forward_moves  = 0;
    int forward_square = pawn_square + (8 * direction);

    if (forward_square >= 0 && forward_square < 64 && !GET_BIT(occupancy, forward_square)) {
      SET_BIT(forward_moves, forward_square);

      if ((color == WHITE && rank == 1) || (color == BLACK && rank == 6)) {
        int double_square = forward_square + (8 * direction);
        if (double_square >= 0 && double_square < 64 && !GET_BIT(occupancy, double_square)) {
          SET_BIT(forward_moves, double_square);
        }
      }
    }

    U64 all_moves  = attacks | forward_moves;
    U64 moves_copy = all_moves;

    while (moves_copy) {
      int target_square = POP_LSB(&moves_copy);
      movelist->moves[movelist->count++] = create_move(pawn_square, target_square);
    }
  }
}

void gen_knights(const BitboardSet *bbset, MoveList *movelist, Turn color) {
  U64 knights         = bbset->knights[color];
  U64 friendly_pieces = bbset->all_pieces[color];

  while (knights) {
    int knight_square = POP_LSB(&knights);
    U64 attacks       = knight_attacks[knight_square] & ~friendly_pieces;
    U64 attacks_copy  = attacks;

    while (attacks_copy) {
      int target_square = POP_LSB(&attacks_copy);
      movelist->moves[movelist->count++] = create_move(knight_square, target_square);
    }
  }
}

// incomplete: castling
void gen_kings(const BitboardSet *bbset, MoveList *movelist, Turn color, uint8_t castling_rights) {
  (void)castling_rights;

  U64 kings           = bbset->kings[color];
  U64 friendly_pieces = bbset->all_pieces[color];
  U64 occupied        = bbset->occupied;
  U64 enemy_attacks   = compute_attacks(bbset, !color);

  while (kings) {
    int king_square  = POP_LSB(&kings);
    U64 attacks      = king_attacks[king_square] & ~friendly_pieces & ~enemy_attacks;
    U64 attacks_copy = attacks;

    while (attacks_copy) {
      int target_square = POP_LSB(&attacks_copy);
      movelist->moves[movelist->count++] = create_move(king_square, target_square);
    }

    /* castling */
    bool legal;

    if (color == WHITE) {
      // white-kingside
      legal = (
       castling_rights & CASTLE_WK  &&
       !(occupied & (1ULL << F1))   &&
       !(occupied & (1ULL << G1))   &&
       !(square_attacked(bbset, E1, BLACK)) &&
       !(square_attacked(bbset, F1, BLACK)) &&
       !(square_attacked(bbset, G1, BLACK))
      );
      if (legal) movelist->moves[movelist->count++] = create_castling(E1, G1);

      // white-queenside
      legal = (
       castling_rights & CASTLE_WQ &&
       !(occupied & (1ULL << B1))  &&
       !(occupied & (1ULL << C1))  &&
       !(occupied & (1ULL << D1))  &&
       !(square_attacked(bbset, E1, BLACK)) &&
       !(square_attacked(bbset, D1, BLACK)) &&
       !(square_attacked(bbset, C1, BLACK))
      );
      if (legal) movelist->moves[movelist->count++] = create_castling(E1, C1);
    }

    else {
      // black-kingside
      legal = (
       castling_rights & CASTLE_BK &&
       !(occupied & (1ULL << F8))  &&
       !(occupied & (1ULL << G8))  &&
       !(square_attacked(bbset, E8, WHITE)) &&
       !(square_attacked(bbset, F8, WHITE)) &&
       !(square_attacked(bbset, G8, WHITE))
      );
      if (legal) movelist->moves[movelist->count++] = create_castling(E8, G8);

      // black-queenside
      legal = (
       castling_rights & CASTLE_BQ &&
       !(occupied & (1ULL << B8))  &&
       !(occupied & (1ULL << C8))  &&
       !(occupied & (1ULL << D8))  &&
       !(square_attacked(bbset, E8, WHITE)) &&
       !(square_attacked(bbset, D8, WHITE)) &&
       !(square_attacked(bbset, C8, WHITE))
      );
      if (legal) movelist->moves[movelist->count++] = create_castling(E8, C8);
    }
  }
}

void gen_bishops(const BitboardSet *bbset, MoveList *movelist, Turn color) {
  (void)bbset; (void)movelist; (void)color;
}

void gen_rooks(const BitboardSet *bbset, MoveList *movelist, Turn color) {
  (void)bbset; (void)movelist; (void)color;
}

void gen_queens(const BitboardSet *bbset, MoveList *movelist, Turn color) {
  (void)bbset; (void)movelist; (void)color;
}

static void attack_table_for_pawn(void) {
  for (int square = 0; square < 64; ++square) {
    pawn_attacks[WHITE][square] = 0;
    pawn_attacks[BLACK][square] = 0;

    int file = square % 8;
    int rank = square / 8;

    if (rank < 7) {
      if (file > 0) SET_BIT(pawn_attacks[WHITE][square], (rank+1) * 8 + (file-1));
      if (file < 7) SET_BIT(pawn_attacks[WHITE][square], (rank+1) * 8 + (file+1));
    }
    if (rank > 0) {
      if (file > 0) SET_BIT(pawn_attacks[BLACK][square], (rank-1) * 8 + (file-1));
      if (file < 7) SET_BIT(pawn_attacks[BLACK][square], (rank-1) * 8 + (file+1));
    }
  }
}

static void attack_table_for_knight(void) {
  int knight_moves[8][2] = {{+2,-1},{+2,+1},
                            {+1,-2},{+1,+2},
                            {-1,-2},{-1,+2},
                            {-2,-1},{-2,+1}};

  for (int square = 0; square < 64; ++square) {
    knight_attacks[square] = 0;
    int file = square % 8;
    int rank = square / 8;

    for (int i = 0; i < 8; ++i) {
      int new_file = file + knight_moves[i][0];
      int new_rank = rank + knight_moves[i][1];

      if (new_file >= 0 && new_file < 8 && new_rank >= 0 && new_rank < 8) {
        int target_square = new_rank * 8 + new_file;
        SET_BIT(knight_attacks[square], target_square);
      }
    }
  }
}

static void attack_table_for_king(void) {
  int king_moves[8][2] = {{+1,+0},{-1,+0},
                          {+0,+1},{+0,-1},
                          {+1,+1},{+1,-1},
                          {-1,+1},{-1,-1}};

  for (int square = 0; square < 64; ++square) {
    king_attacks[square] = 0;
    int file = square % 8;
    int rank = square / 8;

    for (int i = 0; i < 8; ++i) {
      int new_file = file + king_moves[i][0];
      int new_rank = rank + king_moves[i][1];

      if (new_file >= 0 && new_file < 8 && new_rank >= 0 && new_rank < 8) {
        int target_square = new_rank * 8 + new_file;
        SET_BIT(king_attacks[square], target_square);
      }
    }
  }
}

U64 compute_attacks(const BitboardSet *bbset, Turn enemy_color) {
  U64 attacks = 0;
  
  /* == Leaping Pieces == */
  Bitboard pawns = bbset->pawns[enemy_color];
  while (pawns) {
    int square = POP_LSB(&pawns);
    attacks   |= pawn_attacks[enemy_color][square];
  }

  Bitboard knights = bbset->knights[enemy_color];
  while (knights) {
    int square = POP_LSB(&knights);
    attacks   |= knight_attacks[square];
  }

  Bitboard kings = bbset->kings[enemy_color];
  if (kings) {
    int square = GET_LSB(kings);
    attacks   |= king_attacks[square];
  }
  
  /* == Sliding Pieces == */
//  Bitboard bishops = bbset->bishops[enemy_color];
//  while (bishops) {
//    int square = POP_LSB(&bishops);
//    attacks   |= get_bishop_attacks(square, bbset->occupied);
//  }

//  Bitboard rooks = bbset->rooks[enemy_color];
//  while (rooks) {
//    int square = POP_LSB(&rooks);
//    attacks   |= get_rook_attacks(square, bbset->occupied);
//  }

//  Bitboard queens = bbset->queens[enemy_color];
//  while (queens) {
//    int square = POP_LSB(&queens);
//    attacks   |= get_rook_attacks  (square, bbset->occupied) |
//                 get_bishop_attacks(square, bbset->occupied);
//  }

  return attacks;
}

bool square_attacked(const BitboardSet *bbset, Square square, Turn color) {
  U64 pawn_attacks_ = pawn_attacks[!color][square] & bbset->pawns[color];
  if (pawn_attacks_) return true;

  U64 knight_attacks_ = knight_attacks[square] & bbset->knights[color];
  if (knight_attacks_) return true;

  U64 king_attacks_ = king_attacks[square] & bbset->kings[color];
  if (king_attacks_) return true;

  // incomplete: sliding pieces should also be checked.

  return false;
}