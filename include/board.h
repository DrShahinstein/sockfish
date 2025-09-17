#pragma once

#define SQ 100
#define BOARD_SIZE (SQ * 8)
#define PIECE_PATH "assets/pieces/%c.png"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define MAX_HISTORY 128

#include "sockfish/sockfish.h" /* Move, Turn, CASTLE_WK, CASTLE_WQ, CASTLE_BK, CASTLE_BQ, '= Move Utilities =' ... */
#include "sockfish/movegen.h"  /* MoveList, sf_generate_moves() */
#include <SDL3/SDL.h>

typedef struct {
  int from_row;
  int from_col;
  int to_row;
  int to_col;
  char moving_piece;
  char promoted_piece;
  char captured_piece;
  int captured_row;
  int captured_col;
  uint8_t castling;
  int ep_row;
  int ep_col;
  Turn turn;
} BoardMoveHistory;

typedef struct {
  bool active;
  int row;
  int col;
  char choices[4];
  char captured;
} Promotion;

typedef struct {
  bool active;
  int from_row;
  int from_col;
  int to_row;
  int to_col;
} Drag;

typedef struct {
  bool active;
  int row;
  int col;
} SelectedPiece;

typedef struct BoardState {
  char board[8][8];
  uint8_t castling;
  Turn turn;
  MoveList valid_moves;
  SelectedPiece selected_piece;
  Drag drag;
  Promotion promo;
  int ep_row; // en-passant row (-1 for none)
  int ep_col; // en-passant col (-1 for none)
  BoardMoveHistory history[MAX_HISTORY]; int undo_count; int redo_count;
} BoardState ;

void load_fen(const char *fen, BoardState *board);
void load_board(const char *fen, BoardState *board);
void board_init(BoardState *board);
void board_save_history(BoardState *board, int from_row, int from_col, int to_row, int to_col);
void board_undo(BoardState *board);
void board_redo(BoardState *board);