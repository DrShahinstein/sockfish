#pragma once

#define SQ 100
#define BOARD_SIZE (SQ * 8)
#define PIECE_PATH "assets/pieces/%c.png"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"

#include <SDL3/SDL.h>

typedef struct BoardState {
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
} BoardState ;

void load_fen(const char *fen, BoardState *board);
void load_board(const char *fen, BoardState *board);
void load_piece_textures(SDL_Renderer *renderer, BoardState *board);
void board_init(SDL_Renderer *renderer, BoardState *board);
void cleanup_textures(BoardState *board);
void draw_board(SDL_Renderer *renderer, BoardState *board);
