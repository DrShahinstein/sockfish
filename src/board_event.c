#include "board_event.h"
#include "board.h"
#include "window.h"
#include <SDL3/SDL.h>

static bool is_mouse_in_board(float mx, float my);
static void update_castling_rights(BoardState *board, char moving_piece, int from_r, int from_c);
static bool is_castling_move(BoardState *board, int from_r, int from_c, int to_r, int to_c);
static void perform_castling(BoardState *board, int from_r, int from_c, int to_r, int to_c);

void board_handle_event(SDL_Event *e, BoardState *board) {
  float mx; float my; int sq_row; int sq_col;

  switch (e->type) {
  case SDL_EVENT_QUIT:
    board->running = false;
    break;

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    if (e->button.button == SDL_BUTTON_LEFT) {
      SDL_GetMouseState(&mx, &my);

      if (!is_mouse_in_board(mx, my)) {
        board->drag.active = false;
        return;
      }

      sq_col = mx / SQ;
      sq_row = my / SQ;
      char piece = board->board[sq_row][sq_col];

      bool turn_okay = (board->turn == WHITE && SDL_isupper(piece)) || (board->turn == BLACK && SDL_islower(piece));
      if (!turn_okay) {
        board->drag.active=false;
        return;
      }

      if (piece != 0) {
        board->drag.active = true;
        board->drag.row = sq_row;
        board->drag.col = sq_col;
        board->drag.from_row = sq_row;
        board->drag.from_col = sq_col;
      }
    }
    break;

  case SDL_EVENT_MOUSE_BUTTON_UP:
    if (e->button.button == SDL_BUTTON_LEFT) {
      SDL_GetMouseState(&mx, &my);

      if (!is_mouse_in_board(mx, my)) {
        board->drag.active = false;
        return;
      }

      if (board->promo.active) {
        int p = -1;

        for (int i = 0; i < 4; ++i) {
          float choice_x = board->promo.col * SQ;
          float choice_y = board->turn == WHITE ? (i * SQ) : ((7-i) * SQ);
          bool mouse_on_promo = mx >= choice_x && mx < choice_x + SQ && my >= choice_y && my < choice_y + SQ;

          if (mouse_on_promo) {
            p = i;
            break;
          }
        }

        if (p == -1) {
          char p_abort = board->turn == WHITE ? 'P':'p';

          board->drag.active  = false;
          board->promo.active = false;
          board->board[board->drag.from_row][board->drag.from_col] = p_abort;
          board->board[board->promo.row][board->promo.col] = board->promo.captured;
          return;
        }

        char piece = board->promo.choices[p];

        board->board[board->promo.row][board->promo.col] = piece;
        board->turn = (board->turn == WHITE) ? BLACK : WHITE;
        board->promo.active = false;
        board->drag.active = false;
        return;
      }

      if (board->drag.active) {
        if (board->promo.active) {
          board->drag.active = false;
          return;
        }
        
        int from_r = board->drag.from_row;
        int from_c = board->drag.from_col;
        sq_row     = my / SQ; // to row
        sq_col     = mx / SQ; // to col
        char moving_piece = board->board[from_r][from_c];

        bool dropped_same = from_r == sq_row && from_c == sq_col;
        if (dropped_same) {
          board->drag.active = false;
          return;
        }

        if (is_castling_move(board, from_r, from_c, sq_row, sq_col)) {
          perform_castling(board, from_r, from_c, sq_row, sq_col);
          board->turn = (board->turn == WHITE) ? BLACK : WHITE;
          return;
        }

        bool legal = true; // for now

        if (legal) {
          board->promo.captured = board->board[sq_row][sq_col];
          board->board[sq_row][sq_col] = moving_piece;
          board->board[from_r][from_c] = 0;

          update_castling_rights(board, moving_piece, from_r, from_c);

          bool promotion = (sq_row == 0 || sq_row == 7) && (moving_piece == 'p' || moving_piece == 'P');

          if (promotion) {
            board->promo.active = true;
            board->promo.row = sq_row;
            board->promo.col = sq_col;
            
            bool w = board->turn == WHITE;
            board->promo.choices[0] = w ? 'Q':'q';
            board->promo.choices[1] = w ? 'R':'r';
            board->promo.choices[2] = w ? 'N':'n';
            board->promo.choices[3] = w ? 'B':'b';
          }
          
          else {
            if (board->turn == WHITE) board->turn = BLACK;
            else board->turn = WHITE;
          }
        }

        board->drag.active = false;
      }
    }
    break;
  }
}

static bool is_mouse_in_board(float mx, float my) {
  return mx >= 0 && mx < BOARD_SIZE && my >= 0 && my < BOARD_SIZE;
}

static void update_castling_rights(BoardState *board, char moving_piece, int from_r, int from_c) {
  if (moving_piece == 'K') {
    board->castling &= ~(CASTLE_WK | CASTLE_WQ);
  } else if (moving_piece == 'k') {
    board->castling &= ~(CASTLE_BK | CASTLE_BQ);
  } else if (moving_piece == 'R') {
    if (from_r == 7 && from_c == 0) board->castling &= ~CASTLE_WQ;
    else if (from_r == 7 && from_c == 7) board->castling &= ~CASTLE_WK;
  } else if (moving_piece == 'r') {
    if (from_r == 0 && from_c == 0) board->castling &= ~CASTLE_BQ;
    else if (from_r == 0 && from_c == 7) board->castling &= ~CASTLE_BK;
  }
}

static bool is_castling_move(BoardState *board, int from_r, int from_c, int to_r, int to_c) {
  char piece = board->board[from_r][from_c];
  
  if ((piece != 'K' && piece != 'k') || from_r != to_r || SDL_abs(from_c - to_c) != 2) {
    return false;
  }
  
  bool kingside = (to_c > from_c);
  int rook_col  = kingside ? 7 : 0;
  char rook     = (piece == 'K') ? 'R':'r';
  
  if (board->board[from_r][rook_col] != rook) {
    return false;
  }
  
  uint8_t required_right;
  if (piece == 'K') {
    required_right = kingside ? CASTLE_WK : CASTLE_WQ;
  } else {
    required_right = kingside ? CASTLE_BK : CASTLE_BQ;
  }
  
  return (board->castling & required_right) != 0;
}

static void perform_castling(BoardState *board, int from_r, int from_c, int to_r, int to_c) {
  char piece        = board->board[from_r][from_c];
  bool kingside     = (to_c > from_c);
  int rook_from_col = kingside ? 7 : 0;
  int rook_to_col   = kingside ? to_c - 1 : to_c + 1;
  
  // move the king
  board->board[to_r][to_c] = piece;
  board->board[from_r][from_c] = 0;
  
  // move the rook
  char rook = board->board[from_r][rook_from_col];
  board->board[from_r][rook_to_col] = rook;
  board->board[from_r][rook_from_col] = 0;
  
  if (piece == 'K') {
    board->castling &= ~(CASTLE_WK | CASTLE_WQ);
  } else {
    board->castling &= ~(CASTLE_BK | CASTLE_BQ);
  }
}