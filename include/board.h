#pragma once

#include "game.h"

#define SQ 100
#define BOARD_SIZE (SQ * 8)
#define PIECE_PATH "assets/pieces/%c.png"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"

void load_fen(const char *fen, GameState *game);
void load_board(const char *fen, GameState *game);
void load_piece_textures(SDL_Renderer *renderer, GameState *game);
void board_init(SDL_Renderer *renderer, GameState *game);
void cleanup_textures(GameState *game);
void draw_board(SDL_Renderer *renderer, GameState *game);
