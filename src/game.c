#include "game.h"
#include "board.h"
#include <SDL3/SDL.h>

void game(SDL_Renderer *renderer, struct GameState *state) {
  draw_board(renderer, state);
}
