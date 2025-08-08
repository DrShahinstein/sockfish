#pragma once

#include <SDL3/SDL.h>
#include "ui.h"

struct GameState {
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
};

void draw_game(SDL_Renderer *renderer, struct GameState *game_state, UI_State *ui_state);
