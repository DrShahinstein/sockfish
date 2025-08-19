#include "board_event.h"
#include "board.h"
#include "window.h"
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
          board->drag.active = false;
          return;
        }

        char piece = board->promo.choices[p];

        board->board[board->promo.row][board->promo.col] = piece;
        board->turn = (board->turn == WHITE) ? BLACK : WHITE;
        board->promo.active = false;
        board->drag.active = false;
        return;
      }

      sq_col = mx / SQ;
      sq_row = my / SQ;

      if (board->drag.active) {
        if (board->promo.active) {
          board->drag.active = false;
          return;
        }

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

          bool promotion =  (sq_row == 0 || sq_row == 7) && (moving_piece == 'p' || moving_piece == 'P');

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
