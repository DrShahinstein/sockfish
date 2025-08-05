#pragma once

#include <SDL3/SDL.h>

struct GameState {
  bool running;
};

void game(SDL_Renderer *renderer, struct GameState *state);
