#pragma once

#include "board.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define DEJAVU_SANS_MONO "assets/DejaVuSansMono.ttf"

extern SDL_Texture *tex[128];
extern TTF_Font    *coord_font;
extern SDL_Texture *coord_tex_light['z' + 1];
extern SDL_Texture *coord_tex_dark ['z' + 1];

void render_board_init(SDL_Renderer *renderer);
void render_board(SDL_Renderer *renderer, BoardState *board);
void render_board_cleanup(void);