#pragma once

#include "sockfish/sockfish.h"

typedef struct {
  Move moves[256]; // max possible moves in a chess position is =218
  int count;
} MoveList;

typedef struct {
  U64 *attacks;
  U64 mask;
  U64 magic;
  int shift;
} MagicEntry;

extern U64 pawn_attacks[2][64];
extern U64 knight_attacks[64];
extern U64 king_attacks[64];
extern U64 rook_magics[64];   extern MagicEntry rook_magic[64];
extern U64 bishop_magics[64]; extern MagicEntry bishop_magic[64];

void init_attack_tables(void);   // for leaping pieces (pawn, knight, king)
void init_magic_bitboards(void); // for sliding pieces (bishop, rook, queen)

void gen_pawns  (Bitboard pawns,   MoveList *movelist, U64 occupancy,      /*noneed*/      U64 enemy_pieces, Turn pawn_color);
void gen_rooks  (Bitboard rooks,   MoveList *movelist, U64 occupancy, U64 friendly_pieces);
void gen_knights(Bitboard knights, MoveList *movelist,   /*noneed*/   U64 friendly_pieces);
void gen_bishops(Bitboard bishops, MoveList *movelist, U64 occupancy, U64 friendly_pieces);
void gen_queens (Bitboard queens,  MoveList *movelist, U64 occupancy, U64 friendly_pieces);
void gen_kings  (Bitboard kings,   MoveList *movelist,   /*noneed*/   U64 friendly_pieces, U64 enemy_attacks, uint8_t castling_rights);
U64 compute_attacks(const BitboardSet *bbset, Turn enemy_color);
MoveList sf_generate_moves(const BitboardSet *bbset, Turn color, uint8_t castling_rights);

/*

* MagicEntry => https://www.chessprogramming.org/Magic_Bitboards#Fancy
* Precomputed attack tables:
  - Leaping pieces (pawn, knight, king)  have constant move patterns.
  - Sliding pieces (rook, bishop, queen) have variable move patterns depending on the position.
  - For performance reasons, we create precomputed attack tables for all pieces.
  - It's easier to do that for leaping pieces.
  - For sliding pieces, we are getting involved with 'magic' concept.
  - MagicEntry:
      U64 *attacks => sliding pieces have their attack table here in their MagicEntry
      U64 mask     => defines which squares are relevant for calculating attacks.
      U64 magic    => Special 64-bit constants that, when multiplied with occupancy patterns, 
                      produce unique indices into the attack tables through bit shifting
      int shift    => shift amounts to the right

*/