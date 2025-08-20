#pragma once

#define SQ 100
#define BOARD_SIZE (SQ * 8)
#define PIECE_PATH "assets/pieces/%c.png"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#include <SDL3/SDL.h>

#define CASTLE_WK 0x01
#define CASTLE_WQ 0x02
#define CASTLE_BK 0x04
#define CASTLE_BQ 0x08

typedef enum { WHITE, BLACK } Turn;

typedef struct {
  int fr; int fc;
  int tr; int tc;
} Move;

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
} BoardState ;

void load_fen(const char *fen, BoardState *board);
void load_board(const char *fen, BoardState *board);
void load_piece_textures(SDL_Renderer *renderer, BoardState *board);
void board_init(SDL_Renderer *renderer, BoardState *board);
void cleanup_textures(BoardState *board);
void draw_board(SDL_Renderer *renderer, BoardState *board);
