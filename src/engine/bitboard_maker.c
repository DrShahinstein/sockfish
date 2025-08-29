#include "engine.h"

void make_bitboards_from_charboard(const char board[8][8], SF_Context *ctx) {
  for (int color = 0; color < 2; ++color) {
    ctx->bitboard_set.pawns     [color] = 0;
    ctx->bitboard_set.knights   [color] = 0;
    ctx->bitboard_set.bishops   [color] = 0;
    ctx->bitboard_set.rooks     [color] = 0;
    ctx->bitboard_set.queens    [color] = 0;
    ctx->bitboard_set.kings     [color] = 0;
    ctx->bitboard_set.all_pieces[color] = 0;
  }
  ctx->bitboard_set.occupied = 0;

  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      char piece = board[row][col];
      if (piece == 0)
        continue;

      int square = (7 - row) * 8 + col;
      int color  = (piece >= 'A' && piece <= 'Z') ? WHITE : BLACK;
      char piece_lower = SDL_tolower(piece);

      SET_BIT(ctx->bitboard_set.occupied, square);
      SET_BIT(ctx->bitboard_set.all_pieces[color], square);

      switch (piece_lower) {
       case 'p': SET_BIT(ctx->bitboard_set.pawns  [color], square); break;
       case 'n': SET_BIT(ctx->bitboard_set.knights[color], square); break;
       case 'b': SET_BIT(ctx->bitboard_set.bishops[color], square); break;
       case 'r': SET_BIT(ctx->bitboard_set.rooks  [color], square); break;
       case 'q': SET_BIT(ctx->bitboard_set.queens [color], square); break;
       case 'k': SET_BIT(ctx->bitboard_set.kings  [color], square); break;
      }
    }
  }
}