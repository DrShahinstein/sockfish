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

void print_bitboard(Bitboard bb);
int pop_lsb(U64 *bb);
int get_lsb(U64 bb);
int count_bits(U64 bb);