#include "engine.h"

BitboardSet make_bitboards_from_charboard(const char board[8][8]) {
  BitboardSet bbset;

  for (int color = 0; color < 2; ++color) {
    bbset.pawns     [color] = 0;
    bbset.knights   [color] = 0;
    bbset.bishops   [color] = 0;
    bbset.rooks     [color] = 0;
    bbset.queens    [color] = 0;
    bbset.kings     [color] = 0;
    bbset.all_pieces[color] = 0;
  }

  bbset.occupied = 0;

  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      char piece = board[row][col];
      if (piece == 0)
        continue;

      int square       = (7 - row) * 8 + col;
      int color        = (piece >= 'A' && piece <= 'Z') ? WHITE : BLACK;
      char piece_lower = SDL_tolower(piece);

      SET_BIT(bbset.occupied, square);
      SET_BIT(bbset.all_pieces[color], square);

      switch (piece_lower) {
      case 'p': SET_BIT(bbset.pawns  [color], square); break;
      case 'n': SET_BIT(bbset.knights[color], square); break;
      case 'b': SET_BIT(bbset.bishops[color], square); break;
      case 'r': SET_BIT(bbset.rooks  [color], square); break;
      case 'q': SET_BIT(bbset.queens [color], square); break;
      case 'k': SET_BIT(bbset.kings  [color], square); break;
      }
    }
  }

  return bbset;
}