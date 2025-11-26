#pragma once

#define SQ 100
#define BOARD_SIZE (SQ * 8)
#define PIECE_PATH "assets/pieces/%c.png"
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define MAX_HISTORY 512
#define MAX_ARROWS 64
#define MAX_HIGHLIGHTS 64

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

/* Internal Type For 'BoardState' */
typedef struct {
  bool active;
  int row;
  int col;
  char choices[4];
  char captured;
} Promotion;

/* Internal Type For 'BoardState' */
typedef struct {
  bool active;
  int from_row;
  int from_col;
  int to_row;
  int to_col;
} Drag;

/* Internal Type For 'BoardState' */
typedef struct {
  bool active;
  int row;
  int col;
} SelectedPiece;

/* Internal Type For 'BoardState' */
typedef struct {
  bool in_check;
  Turn color;
  int row;
  int col;
} King;

/* Internal Type For 'Annotations' */
typedef struct {
  Square from;
  Square to;
  SDL_FColor color;
} Arrow;

/* Internal Type For 'Annotations' */
typedef struct {
  Square square;
  SDL_FColor color;
} Highlight;

/* Internal Type For 'BoardState' */
typedef struct {
  Square arrow_start;                   bool drawing_arrow;
  Arrow arrows[MAX_ARROWS];             int arrow_count;
  Highlight highlights[MAX_HIGHLIGHTS]; int highlight_count;
} Annotations;

typedef struct BoardState {
  char board[8][8];
  int ep_row;
  int ep_col;
  uint8_t castling;
  Turn turn;
  MoveList valid_moves; bool should_update_valid_moves;
  SelectedPiece selected_piece;
  Drag drag;
  Promotion promo;
  King king;
  BoardMoveHistory history[MAX_HISTORY]; int undo_count; int redo_count;
  Annotations annotations;
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

// parse_pgn_move.c
void parse_pgn_move(const char *pgnmove, SF_Context *sf_ctx, char (*last_pos)[8], char *promote, int *fr, int *fc, int *tr, int *tc);

// special_moves.c
void update_castling_rights(BoardState *board, char moving_piece, Move move);
bool is_castling_move(BoardState *board, Move move);
void perform_castling(BoardState *board, Move move);
bool is_en_passant_capture(BoardState *board, Move move);
void update_enpassant_rights(BoardState *board, char moving_piece);

// annotations.c
void clear_annotations(Annotations *anns);
void start_drawing_arrow(Annotations *anns, Square start);
void cancel_drawing_arrow(Annotations *anns);
bool is_drawing_arrow(const Annotations *anns);
Square get_arrow_start(const Annotations *anns);
int get_arrow_count(const Annotations *anns);
int get_highlight_count(const Annotations *anns);
bool has_arrow(Annotations *anns, Square from, Square to);
bool add_arrow(Annotations *anns, Square from, Square to, SDL_FColor color);
bool remove_arrow(Annotations *anns, Square from, Square to);
bool add_highlight(Annotations *anns, Square square, SDL_FColor color);
bool remove_highlight(Annotations *anns, Square square);
bool has_highlight(Annotations *anns, Square square);