#include "board.h"
#include <SDL3/SDL.h>

void draw_board(SDL_Renderer *renderer, struct GameState *state) {
  for (int row = 0; row < 8; ++row) {
    for (int col = 0; col < 8; ++col) {
      SDL_FRect square = {
        .x = col * SQ,
        .y = row * SQ,
        .w = SQ,
        .h = SQ
      };

      if ((row + col) % 2 == 0) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      } else {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      }
      SDL_RenderFillRect(renderer, &square);
    }
  }

  SDL_RenderPresent(renderer);
}
