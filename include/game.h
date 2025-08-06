#pragma once

#include <SDL3/SDL.h>

struct GameState {
  bool running;
  char board[8][8];
  SDL_Texture *tex[128];
};

void draw_game(SDL_Renderer *renderer, struct GameState *state);
