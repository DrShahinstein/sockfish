#pragma once

#define SQ 100
#define BOARD_SIZE (SQ * 8)
#define PIECE_PATH "assets/pieces/%c.png"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#include "sockfish.h" /* Move, Turn */
#include <SDL3/SDL.h>

#define CASTLE_WK 0x01
#define CASTLE_WQ 0x02
#define CASTLE_BK 0x04
#define CASTLE_BQ 0x08

typedef struct {
  bool active;
  int row;
  int col;
  char choices[4];
  char captured;
} Promotion;

typedef struct BoardState {
  bool running;
  char board[8][8];
  uint8_t castling;
  Turn turn;
  SDL_Texture *tex[128];
  struct {
    int row;
    int col;
    int from_row;
    int from_col;
    bool active;
  } drag;
  Promotion promo;
  int ep_row; // en-passant row (-1 for none)
  int ep_col; // en-passant col (-1 for none)
} BoardState ;

void load_fen(const char *fen, BoardState *board);
void load_board(const char *fen, BoardState *board);
void load_piece_textures(SDL_Renderer *renderer, BoardState *board);
void board_init(SDL_Renderer *renderer, BoardState *board);
void cleanup_textures(BoardState *board);
void draw_board(SDL_Renderer *renderer, BoardState *board);
