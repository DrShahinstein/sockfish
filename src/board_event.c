#include "board_event.h"
#include "board.h"
#include <SDL3/SDL.h>

static bool is_mouse_in_board(float mx, float my);

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

      sq_col = mx / SQ;
      sq_row = my / SQ;

      if (board->drag.active) {
        int from_r = board->drag.from_row;
        int from_c = board->drag.from_col;
        char moving_piece = board->board[from_r][from_c];
        bool legal = true; // for now
        bool dropped_same = from_r == sq_row && from_c == sq_col;
        
        if (dropped_same) {
          board->drag.active = false;
          return;
        }

        if (legal) {
          board->board[sq_row][sq_col] = moving_piece;
          board->board[from_r][from_c] = 0;
          
          if (board->turn == WHITE) board->turn = BLACK;
          else board->turn = WHITE;
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
