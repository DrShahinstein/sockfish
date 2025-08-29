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