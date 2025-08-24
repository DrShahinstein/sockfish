#pragma once

#include "board.h"
#include <SDL3/SDL.h>

void render_board_init(SDL_Renderer *renderer, BoardState *board);
void render_board(SDL_Renderer *renderer, BoardState *board);
void render_board_cleanup(SDL_Renderer *renderer, BoardState *board);