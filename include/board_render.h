#pragma once

#include "board.h"
#include <SDL3/SDL.h>

extern SDL_Texture *tex[128];

void render_board_init(SDL_Renderer *renderer);
void render_board(SDL_Renderer *renderer, BoardState *board);
void render_board_cleanup(void);