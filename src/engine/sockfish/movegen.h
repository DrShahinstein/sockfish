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

void init_attack_tables(void);      // for leaping pieces (pawn, knight, king)
void init_magic_bitboards(void);    // for sliding pieces (bishop, rook, queen)
void cleanup_magic_bitboards(void);

void gen_pawns  (const BitboardSet *bbset, MoveList *movelist, Turn color, Square enpassant_sq);
void gen_rooks  (const BitboardSet *bbset, MoveList *movelist, Turn color);
void gen_knights(const BitboardSet *bbset, MoveList *movelist, Turn color);
void gen_bishops(const BitboardSet *bbset, MoveList *movelist, Turn color);
void gen_queens (const BitboardSet *bbset, MoveList *movelist, Turn color);
void gen_kings  (const BitboardSet *bbset, MoveList *movelist, Turn color, uint8_t castling_rights);

bool square_attacked(const BitboardSet *bbset, Square square, Turn color);
U64 compute_attacks(const BitboardSet *bbset, Turn enemy_color);

MoveList sf_generate_moves(const SF_Context *ctx);

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


/* == MAGIC NUMBERS == */
static const U64 MAGIC_NUMBERS_FOR_ROOK[64] = {
  0x8a80104000800020ULL,
  0x140002000100040ULL,
  0x2801880a0017001ULL,
  0x100081001000420ULL,
  0x200020010080420ULL,
  0x3001c0002010008ULL,
  0x8480008002000100ULL,
  0x2080088004402900ULL,
  0x800098204000ULL,
  0x2024401000200040ULL,
  0x100802000801000ULL,
  0x120800800801000ULL,
  0x208808088000400ULL,
  0x2802200800400ULL,
  0x2200800100020080ULL,
  0x801000060821100ULL,
  0x80044006422000ULL,
  0x100808020004000ULL,
  0x12108a0010204200ULL,
  0x140848010000802ULL,
  0x481828014002800ULL,
  0x8094004002004100ULL,
  0x4010040010010802ULL,
  0x20008806104ULL,
  0x100400080208000ULL,
  0x2040002120081000ULL,
  0x21200680100081ULL,
  0x20100080080080ULL,
  0x2000a00200410ULL,
  0x20080800400ULL,
  0x80088400100102ULL,
  0x80004600042881ULL,
  0x4040008040800020ULL,
  0x440003000200801ULL,
  0x4200011004500ULL,
  0x188020010100100ULL,
  0x14800401802800ULL,
  0x2080040080800200ULL,
  0x124080204001001ULL,
  0x200046502000484ULL,
  0x480400080088020ULL,
  0x1000422010034000ULL,
  0x30200100110040ULL,
  0x100021010009ULL,
  0x2002080100110004ULL,
  0x202008004008002ULL,
  0x20020004010100ULL,
  0x2048440040820001ULL,
  0x101002200408200ULL,
  0x40802000401080ULL,
  0x4008142004410100ULL,
  0x2060820c0120200ULL,
  0x1001004080100ULL,
  0x20c020080040080ULL,
  0x2935610830022400ULL,
  0x44440041009200ULL,
  0x280001040802101ULL,
  0x2100190040002085ULL,
  0x80c0084100102001ULL,
  0x4024081001000421ULL,
  0x20030a0244872ULL,
  0x12001008414402ULL,
  0x2006104900a0804ULL,
  0x1004081002402ULL,
};