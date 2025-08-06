#include "game.h"
#include "board.h"
#include <SDL3/SDL.h>

void draw_game(SDL_Renderer *renderer, struct GameState *state) {
  draw_board(renderer, state);
}
