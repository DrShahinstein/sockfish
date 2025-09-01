#pragma once

#include <stdint.h>

typedef uint64_t U64;
typedef uint64_t Bitboard;
typedef struct {
  Bitboard pawns[2];
  Bitboard knights[2];
  Bitboard bishops[2];
  Bitboard rooks[2];
  Bitboard queens[2];
  Bitboard kings[2];
  Bitboard all_pieces[2];
  Bitboard occupied;    
} BitboardSet;

#define SET_BIT(bb, square)   ((bb) |= (1ULL << (square)))
#define CLEAR_BIT(bb, square) ((bb) &= ~(1ULL << (square)))
#define GET_BIT(bb, square)   ((bb) & (1ULL << (square)))
#define POP_BIT(bb, square)   (GET_BIT(bb, square) ? (CLEAR_BIT(bb, square), 1) : 0)

#define GET_LSB(bb)    (__builtin_ctzll(bb))
#define GET_MSB(bb)    (__builtin_clzll(bb))
#define COUNT_BITS(bb) (__builtin_popcountll(bb))

static inline int POP_LSB(U64 *bb) {
  if (*bb == 0) return -1;
  int index = GET_LSB(*bb);
  *bb &= *bb - 1;
  return index;
}

void print_bitboard(Bitboard bb);