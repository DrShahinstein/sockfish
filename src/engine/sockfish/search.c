#include "sockfish/search.h"
#include "sockfish/movegen.h"
#include <stdio.h> //temp

/* skeleton code for testing movegens */
Move sf_search(const SF_Context *ctx) {
  (void)ctx;

  static const char *square_names[] = { //temp
    "a1","b1","c1","d1","e1","f1","g1","h1", //temp
    "a2","b2","c2","d2","e2","f2","g2","h2", //temp
    "a3","b3","c3","d3","e3","f3","g3","h3", //temp
    "a4","b4","c4","d4","e4","f4","g4","h4", //temp
    "a5","b5","c5","d5","e5","f5","g5","h5", //temp
    "a6","b6","c6","d6","e6","f6","g6","h6", //temp
    "a7","b7","c7","d7","e7","f7","g7","h7", //temp
    "a8","b8","c8","d8","e8","f8","g8","h8"  //temp
  };

  printf("%d\n", ctx->enpassant_sq);
  MoveList generated_moves = sf_generate_moves(&ctx->bitboard_set, ctx->search_color, ctx->castling_rights, ctx->enpassant_sq);

  for (int i = 0; i < generated_moves.count; ++i) {
    Move move = generated_moves.moves[i];
    Square from = move_from(move);
    Square to   = move_to(move);
  
    printf("Move %d: from %s to %s\n", i+1, square_names[from], square_names[to]);
  }

  printf("\n=> Generated for %s\n", ctx->search_color == WHITE ? "WHITE" : "BLACK");

  return create_move(E2, E4);
}