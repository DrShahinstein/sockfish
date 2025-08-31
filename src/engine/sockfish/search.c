#include "sockfish/search.h"
#include "sockfish/movegen.h"
#include <stdio.h> //temp

/* skeleton code for testing movegens */
Move sf_search(const SF_Context *ctx) {
  (void)ctx;

  MoveList generated_moves = sf_generate_moves(&ctx->bitboard_set, ctx->search_color);

  for (int i = 0; i < generated_moves.count; ++i) {
    Move move = generated_moves.moves[i];
    printf("Move %d: from (%d,%d) to (%d,%d)\n", i+1, move.fr, move.fc, move.tr, move.tc);
  }

  printf("\n=> Generated for %s\n", ctx->search_color == WHITE ? "WHITE" : "BLACK");

  return (Move){0};
}