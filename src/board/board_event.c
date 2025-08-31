#include "board_event.h"
#include "board.h"
#include "window.h"
#include "special_moves.h"
#include <SDL3/SDL.h>

static bool is_mouse_in_board(float mx, float my);

void board_handle_event(SDL_Event *e, BoardState *board) {
  float mx; float my; int sq_row; int sq_col;

  switch (e->type) {
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
        board->drag.active = false;
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

        MoveRC move;
        move.fr = board->drag.from_row;
        move.fc = board->drag.from_col;
        move.tr = my / SQ;
        move.tc = mx / SQ;
        char moving_piece = board->board[move.fr][move.fc];

        bool dropped_same = move.fr == move.tr && move.fc == move.tc;
        if (dropped_same) {
          board->drag.active = false;
          return;
        }

        if (is_castling_move(board, &move)) {
          perform_castling(board, &move);
          board->turn = (board->turn == WHITE) ? BLACK : WHITE;
          return;
        }

        if (is_en_passant_capture(board, &move)) {
          int captured_row = board->turn == WHITE ? move.tr + 1 : move.tr - 1;
          board->board[captured_row][move.tc] = 0;
          board->board[move.tr][move.tc] = moving_piece;
          board->board[move.fr][move.fc] = 0;
          board->ep_row = -1;
          board->ep_col = -1;
          board->turn = (board->turn == WHITE) ? BLACK : WHITE;
          board->drag.active = false;
          return;
        }

        bool legal = true; // for now

        if (legal) {
          if ((moving_piece == 'p' || moving_piece == 'P') && SDL_abs(move.fr - move.tr) == 2) {
            board->ep_row = (move.fr + move.tr) / 2;
            board->ep_col = move.fc;
          } else {
            board->ep_row = -1;
            board->ep_col = -1;
          }
          board->promo.captured = board->board[move.tr][move.tc];
          board->board[move.tr][move.tc] = moving_piece;
          board->board[move.fr][move.fc] = 0;

          update_castling_rights(board, moving_piece, &move);
          update_enpassant_rights(board, moving_piece);

          bool promotion = (move.tr == 0 || move.tr == 7) && (moving_piece == 'p' || moving_piece == 'P');

          if (promotion) {
            board->promo.active = true;
            board->promo.row    = move.tr;
            board->promo.col    = move.tc;

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