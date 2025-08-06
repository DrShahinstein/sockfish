#pragma once

#include <SDL3/SDL.h>

typedef struct {
  int row;
  int col;
  int from_row;
  int from_col;
  bool active;
} Drag;

struct GameState {
  bool running;
  char board[8][8];
  SDL_Texture *tex[128];
  Drag drag;
};

void draw_game(SDL_Renderer *renderer, struct GameState *state);
