#pragma once

#include <SDL3/SDL.h>

struct GameState {
  bool running;
};

void draw_game(SDL_Renderer *renderer, struct GameState *state);
