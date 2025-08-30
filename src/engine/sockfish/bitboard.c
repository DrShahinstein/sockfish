#include "bitboard.h"
#include <stdio.h>

void print_bitboard(Bitboard bb) {
  for (int rank = 7; rank >= 0; rank--) {
    printf("%d ", rank + 1);
    for (int file = 0; file < 8; file++) {
      int square = rank * 8 + file;
      printf(" %c", (bb & (1ULL << square)) ? '1' : '.');
    }
    printf("\n");
  }
  printf("   a b c d e f g h\n");
}

inline int pop_lsb(U64 *bb) {
  if (*bb == 0) return -1;
  int index = __builtin_ctzll(*bb);
  *bb &= *bb - 1;
  return index;
}

inline int get_lsb(U64 bb) {
  if (bb == 0) return -1;
  return __builtin_ctzll(bb);
}

inline int count_bits(U64 bb) {
  return __builtin_popcountll(bb);
}