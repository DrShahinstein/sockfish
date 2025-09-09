#include "sockfish/movegen.h"
#include <stdlib.h>
#include <string.h>

MagicEntry rook_magic[64];
MagicEntry bishop_magic[64];

U64 pawn_attacks[2][64];
U64 knight_attacks[64];
U64 king_attacks[64];
U64 rook_magics[64];
U64 bishop_magics[64] = {0};

static void attack_table_for_pawn(void);
static void attack_table_for_knight(void);
static void attack_table_for_king(void);
void init_attack_tables(void) {
  attack_table_for_pawn();
  attack_table_for_knight();
  attack_table_for_king();
}

static U64 rook_mask             (Square square);
static U64 compute_rook_attacks  (Square square, U64 occupancy);
static U64 get_rook_attacks      (Square square, U64 occupancy);
static U64 bishop_mask           (Square square);
static U64 compute_bishop_attacks(Square square, U64 occupancy);
static U64 get_bishop_attacks    (Square square, U64 occupancy);
void init_magic_bitboards(void) {
  memcpy(rook_magics, MAGIC_NUMBERS_FOR_ROOK, sizeof(rook_magics));
  memcpy(bishop_magics, MAGIC_NUMBERS_FOR_BISHOP, sizeof(bishop_magics));

  /* - Initialize Rook Magic Entries - */
  for (Square square = 0; square < 64; square++) {
    U64 mask = rook_mask(square);
    int bits = COUNT_BITS(mask);

    rook_magic[square].mask    = mask;
    rook_magic[square].magic   = rook_magics[square];
    rook_magic[square].shift   = 64 - bits;
    rook_magic[square].attacks = (U64 *)malloc((1 << bits) * sizeof(U64));

    U64 occupancy = 0;
    do {
      U64 attacks                       = compute_rook_attacks(square, occupancy);
      U64 index                         = (occupancy * rook_magic[square].magic) >> rook_magic[square].shift;
      rook_magic[square].attacks[index] = attacks;
      occupancy                         = (occupancy - mask) & mask;
    } while (occupancy);
  }

  /* - Initialize Bishop Magic Entries - */
  for (Square square = 0; square < 64; square++) {
    U64 mask = bishop_mask(square);
    int bits = COUNT_BITS(mask);

    bishop_magic[square].mask    = mask;
    bishop_magic[square].magic   = bishop_magics[square];
    bishop_magic[square].shift   = 64 - bits;
    bishop_magic[square].attacks = (U64 *)malloc((1 << bits) * sizeof(U64));

    U64 occupancy = 0;
    do {
      U64 attacks                         = compute_bishop_attacks(square, occupancy);
      U64 index                           = (occupancy * bishop_magic[square].magic) >> bishop_magic[square].shift;
      bishop_magic[square].attacks[index] = attacks;
      occupancy                           = (occupancy - mask) & mask;
    } while (occupancy);
  }

  /* ! queen is simply rook+bishop ! */

  return;
}

void cleanup_magic_bitboards(void) {
  for (int square = 0; square < 64; square++) {
    if (rook_magic[square].attacks) {
      free(rook_magic[square].attacks);
      rook_magic[square].attacks = NULL;
    }
    if (bishop_magic[square].attacks) {
      free(bishop_magic[square].attacks);
      bishop_magic[square].attacks = NULL;
    }
  }
}

MoveList sf_generate_moves(const SF_Context *ctx) {
  MoveList movelist;
  movelist.count = 0;

  const BitboardSet *bbset = &ctx->bitboard_set;
  Turn color               =  ctx->search_color;
  uint8_t castling_rights  =  ctx->castling_rights;
  Square enpassant_sq      =  ctx->enpassant_sq;

  gen_pawns  (bbset, &movelist, color, enpassant_sq);
  gen_rooks  (bbset, &movelist, color);
  gen_knights(bbset, &movelist, color);
  gen_bishops(bbset, &movelist, color);
  gen_queens (bbset, &movelist, color);
  gen_kings  (bbset, &movelist, color, castling_rights);

  return movelist;
}

void gen_pawns(const BitboardSet *bbset, MoveList *movelist, Turn color, Square enpassant_sq) {
  U64 pawns        = bbset->pawns[color];
  U64 enemy_pieces = bbset->all_pieces[!color];
  U64 occupancy    = bbset->occupied;
  int direction    = (color == WHITE) ? +1 : -1;
  int promo_rank   = (color == WHITE) ? +7 : +0;
  int start_rank   = (color == WHITE) ? +1 : +6;

  while (pawns) {
    int pawn_square    = POP_LSB(&pawns);
    int forward_square = pawn_square + (8 * direction);
    int rank           = pawn_square / 8;

    /* - Moving - */
    if (forward_square >= 0 && forward_square < 64 && !GET_BIT(occupancy, forward_square)) {
      if (forward_square / 8 == promo_rank) {
        movelist->moves[movelist->count++] = create_promotion(pawn_square, forward_square, PROMOTE_QUEEN);
        movelist->moves[movelist->count++] = create_promotion(pawn_square, forward_square, PROMOTE_ROOK);
        movelist->moves[movelist->count++] = create_promotion(pawn_square, forward_square, PROMOTE_BISHOP);
        movelist->moves[movelist->count++] = create_promotion(pawn_square, forward_square, PROMOTE_KNIGHT);
      } else {
        movelist->moves[movelist->count++] = create_move(pawn_square, forward_square);

        // check double push
        if (rank == start_rank) {
          int double_square = forward_square + (8 * direction);
          if (double_square >= 0 && double_square < 64 && !GET_BIT(occupancy, double_square)) {
            movelist->moves[movelist->count++] = create_move(pawn_square, double_square);
          }
        }
      }
    }

    /* - Capturing - */
    U64 attacks = pawn_attacks[color][pawn_square] & enemy_pieces;

    while (attacks) {
      int target_square = POP_LSB(&attacks);

      if (target_square / 8 == promo_rank) {
        movelist->moves[movelist->count++] = create_promotion(pawn_square, target_square, PROMOTE_QUEEN);
        movelist->moves[movelist->count++] = create_promotion(pawn_square, target_square, PROMOTE_ROOK);
        movelist->moves[movelist->count++] = create_promotion(pawn_square, target_square, PROMOTE_BISHOP);
        movelist->moves[movelist->count++] = create_promotion(pawn_square, target_square, PROMOTE_KNIGHT);
      } else {
        movelist->moves[movelist->count++] = create_move(pawn_square, target_square);
      }
    }

    if (enpassant_sq >= 0 && enpassant_sq < 64) {
      U64 ep_attack = pawn_attacks[color][pawn_square] & (1ULL << enpassant_sq);
      if (ep_attack) movelist->moves[movelist->count++] = create_en_passant(pawn_square, enpassant_sq);
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

void gen_kings(const BitboardSet *bbset, MoveList *movelist, Turn color, uint8_t castling_rights) {
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
  U64 bishops  = bbset->bishops[color];
  U64 friendly = bbset->all_pieces[color];
  U64 occupied = bbset->occupied;

  while (bishops) {
    Square bishop_square = POP_LSB(&bishops);
    U64 attacks = get_bishop_attacks(bishop_square, occupied) & ~friendly;

    while (attacks) {
      Square target_square = POP_LSB(&attacks);
      movelist->moves[movelist->count++] = create_move(bishop_square, target_square);
    }
  }
}

void gen_rooks(const BitboardSet *bbset, MoveList *movelist, Turn color) {
  U64 rooks    = bbset->rooks[color];
  U64 friendly = bbset->all_pieces[color];
  U64 occupied = bbset->occupied;

  while (rooks) {
    Square rook_square = POP_LSB(&rooks);
    U64 attacks        = get_rook_attacks(rook_square, occupied) & ~friendly;

    while (attacks) {
      Square target_square = POP_LSB(&attacks);
      movelist->moves[movelist->count++] = create_move(rook_square, target_square);
    }
  }
}

void gen_queens(const BitboardSet *bbset, MoveList *movelist, Turn color) {
  U64 queens   = bbset->queens[color];
  U64 friendly = bbset->all_pieces[color];
  U64 occupied = bbset->occupied;

  while (queens) {
    Square queen_square = POP_LSB(&queens);
    U64 rook_attacks    = get_rook_attacks(queen_square, occupied);
    U64 bishop_attacks  = get_bishop_attacks(queen_square, occupied);
    U64 attacks         = (rook_attacks | bishop_attacks) & ~friendly;

    while (attacks) {
      Square target_square = POP_LSB(&attacks);
      movelist->moves[movelist->count++] = create_move(queen_square, target_square);
    }
  }
}

/* --- */

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

/* --- */

static U64 rook_mask(Square square) {
  U64 mask = 0;
  int r = square / 8, c = square % 8;

  for (int i = r + 1; i <= 6; i++) mask |= (1ULL << (i * 8 + c));
  for (int i = r - 1; i >= 1; i--) mask |= (1ULL << (i * 8 + c));
  for (int j = c + 1; j <= 6; j++) mask |= (1ULL << (r * 8 + j));
  for (int j = c - 1; j >= 1; j--) mask |= (1ULL << (r * 8 + j));

  return mask;
}

static U64 compute_rook_attacks(Square square, U64 occupancy) {
  U64 attacks = 0;
  int r = square / 8, c = square % 8;

  for (int i = r + 1; i < 8; i++) {
    attacks |= (1ULL << (i * 8 + c));
    if (occupancy & (1ULL << (i * 8 + c))) break;
  }
  for (int i = r - 1; i >= 0; i--) {
    attacks |= (1ULL << (i * 8 + c));
    if (occupancy & (1ULL << (i * 8 + c))) break;
  }
  for (int j = c + 1; j < 8; j++) {
    attacks |= (1ULL << (r * 8 + j));
    if (occupancy & (1ULL << (r * 8 + j))) break;
  }
  for (int j = c - 1; j >= 0; j--) {
    attacks |= (1ULL << (r * 8 + j));
    if (occupancy & (1ULL << (r * 8 + j))) break;
  }

  return attacks;
}

static U64 get_rook_attacks(Square square, U64 occupancy) {
  MagicEntry *m          = &rook_magic[square];
  U64 relevant_occupancy = occupancy & m->mask;
  U64 index              = (relevant_occupancy * m->magic) >> m->shift;
  return m->attacks[index];
}

/* --- */

static U64 bishop_mask(Square square) {
  U64 mask = 0;
  int r = square / 8, c = square % 8;

  for (int i = r + 1, j = c + 1; i <= 6 && j <= 6; i++, j++)
    mask |= (1ULL << (i * 8 + j));
  for (int i = r + 1, j = c - 1; i <= 6 && j >= 1; i++, j--)
    mask |= (1ULL << (i * 8 + j));
  for (int i = r - 1, j = c + 1; i >= 1 && j <= 6; i--, j++)
    mask |= (1ULL << (i * 8 + j));
  for (int i = r - 1, j = c - 1; i >= 1 && j >= 1; i--, j--)
    mask |= (1ULL << (i * 8 + j));

  return mask;
}

static U64 compute_bishop_attacks(Square square, U64 occupancy) {
  U64 attacks = 0;
  int r = square / 8, c = square % 8;

  for (int i = r + 1, j = c + 1; i < 8 && j < 8; i++, j++) {
    attacks |= (1ULL << (i * 8 + j));
    if (occupancy & (1ULL << (i * 8 + j))) break;
  }
  for (int i = r + 1, j = c - 1; i < 8 && j >= 0; i++, j--) {
    attacks |= (1ULL << (i * 8 + j));
    if (occupancy & (1ULL << (i * 8 + j))) break;
  }
  for (int i = r - 1, j = c + 1; i >= 0 && j < 8; i--, j++) {
    attacks |= (1ULL << (i * 8 + j));
    if (occupancy & (1ULL << (i * 8 + j))) break;
  }
  for (int i = r - 1, j = c - 1; i >= 0 && j >= 0; i--, j--) {
    attacks |= (1ULL << (i * 8 + j));
    if (occupancy & (1ULL << (i * 8 + j))) break;
  }

  return attacks;
}

static U64 get_bishop_attacks(Square square, U64 occupancy) {
  MagicEntry *m          = &bishop_magic[square];
  U64 relevant_occupancy = occupancy & m->mask;
  U64 index              = (relevant_occupancy * m->magic) >> m->shift;
  return m->attacks[index];
}

/* --- */

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
  Bitboard bishops = bbset->bishops[enemy_color];
  while (bishops) {
    int square = POP_LSB(&bishops);
    attacks   |= get_bishop_attacks(square, bbset->occupied);
  }

  Bitboard rooks = bbset->rooks[enemy_color];
  while (rooks) {
    int square = POP_LSB(&rooks);
    attacks   |= get_rook_attacks(square, bbset->occupied);
  }

  Bitboard queens = bbset->queens[enemy_color];
  while (queens) {
    int square = POP_LSB(&queens);
    attacks   |= get_rook_attacks  (square, bbset->occupied) |
                 get_bishop_attacks(square, bbset->occupied);
  }

  return attacks;
}

bool square_attacked(const BitboardSet *bbset, Square square, Turn color) {
  U64 pawn_attacks_ = pawn_attacks[!color][square] & bbset->pawns[color];
  if (pawn_attacks_) return true;

  U64 knight_attacks_ = knight_attacks[square] & bbset->knights[color];
  if (knight_attacks_) return true;

  U64 king_attacks_ = king_attacks[square] & bbset->kings[color];
  if (king_attacks_) return true;

  U64 bishop_attacks_ = get_bishop_attacks(square, bbset->occupied) & (bbset->bishops[color] | bbset->queens[color]);
  if (bishop_attacks_) return true;

  U64 rook_attacks_ = get_rook_attacks(square, bbset->occupied) & (bbset->rooks[color] | bbset->queens[color]);
  if (rook_attacks_) return true;

  /* ! queen is simply rook+bishop ! */

  return false;
}