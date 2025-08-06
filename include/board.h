#pragma once

#include "game.h"

#define SQ 100
#define PIECE_PATH "assets/pieces/%c.png"

void load_fen(const char *fen, struct GameState *state);
void load_piece_textures(SDL_Renderer *renderer, struct GameState *state);
void initialize_board(SDL_Renderer *renderer, struct GameState *state);
void cleanup_textures(struct GameState *state);
void draw_board(SDL_Renderer *renderer, struct GameState *state);
