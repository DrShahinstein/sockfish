#pragma once

#define SQ 100
#define BOARD_SIZE (SQ * 8)
#define PIECE_PATH "assets/pieces/%c.png"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define MAX_HISTORY 512

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
  MoveList valid_moves; bool should_update_valid_moves;
  SelectedPiece selected_piece;
  Drag drag;
  Promotion promo;
  int ep_row;                                                                     // en-passant row (-1 for none)
  int ep_col;                                                                     // en-passant col (-1 for none)
  BoardMoveHistory history[MAX_HISTORY]; int undo_count; int redo_count;
  struct { bool in_check; Turn color; int row; int col; } king;
} BoardState ;

void board_init(BoardState *board);
void board_handle_event(SDL_Event *e, BoardState *board);
void board_update_king_in_check(BoardState *b);
void board_update_valid_moves(BoardState *b);
void board_save_history(BoardState *board, int from_row, int from_col, int to_row, int to_col, int history_index);
void board_undo(BoardState *board);
void board_redo(BoardState *board);

void load_fen(const char *fen, BoardState *board);
void load_pgn(const char *pgn, BoardState *board);
void parse_pgn_move(const char *pgnmove, SF_Context *sf_ctx, char (*last_pos)[8], char *promote, int *fr, int *fc, int *tr, int *tc); // board/parse_pgn_move.c