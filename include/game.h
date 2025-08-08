#pragma once

#include "ui.h"
#include <SDL3/SDL.h>

typedef struct {
  bool running;
  char board[8][8];
  SDL_Texture *tex[128];
  struct {
    int row;
    int col;
    int from_row;
    int from_col;
    bool active;
  } drag;
} GameState ;

void draw_game(SDL_Renderer *renderer, GameState *game, UI_State *ui);
