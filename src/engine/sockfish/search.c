#include "sockfish/search.h"
#include "sockfish/movegen.h"
#include <stdio.h> //temp

/* skeleton code for testing movegens */
MoveSQ sf_search(const SF_Context *ctx) {
  (void)ctx;

  MoveList generated_moves = sf_generate_moves(&ctx->bitboard_set, ctx->search_color);

  for (int i = 0; i < generated_moves.count; ++i) {
    MoveSQ move = generated_moves.moves[i];
    printf("Move %d: from (%d) to (%d)\n", i+1, move.from, move.to);
  }

  printf("\n=> Generated for %s\n", ctx->search_color == WHITE ? "WHITE" : "BLACK");

  return (MoveSQ){E2,E4};
}