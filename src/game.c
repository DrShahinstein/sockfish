#include "game.h"
#include <SDL3/SDL.h>

void game(SDL_Renderer *renderer, struct GameState *state) {
  (void)state;
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);
}
