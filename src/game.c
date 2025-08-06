#include "game.h"
#include "board.h"
#include <SDL3/SDL.h>

void draw_game(SDL_Renderer *renderer, struct GameState *state) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  draw_board(renderer, state);
  SDL_RenderPresent(renderer);
}
