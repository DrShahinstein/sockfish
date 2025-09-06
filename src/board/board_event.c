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
        board->drag.active   = true;
        board->drag.row      = sq_row;
        board->drag.col      = sq_col;
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
        board->drag.active  = false;
        return;
      }

      if (board->drag.active) {
        if (board->promo.active) {
          board->drag.active = false;
          return;
        }

        int fr            = board->drag.from_row; // from row
        int fc            = board->drag.from_col; // from col
        int tr            = my / SQ;              // to row
        int tc            = mx / SQ;              // to col
        Square from_sq    = rowcol_to_sq(fr, fc);
        Square to_sq      = rowcol_to_sq(tr, tc);
        Move move         = create_move(from_sq, to_sq);
        char moving_piece = board->board[fr][fc];

        bool dropped_same = fr == tr && fc == tc;
        if (dropped_same) {
          board->drag.active = false;
          return;
        }

        if (is_castling_move(board, move)) {
          perform_castling(board, move);
          board->turn = (board->turn == WHITE) ? BLACK : WHITE;
          return;
        }

        if (is_en_passant_capture(board, move)) {
          int captured_row = board->turn == WHITE ? (tr + 1) : (tr - 1);
          board->board[captured_row][tc] = 0;
          board->board[tr][tc] = moving_piece;
          board->board[fr][fc] = 0;
          board->ep_row = NO_ENPASSANT;
          board->ep_col = NO_ENPASSANT;
          board->turn = (board->turn == WHITE) ? BLACK : WHITE;
          board->drag.active = false;
          return;
        }

        bool legal = true; // for now

        if (legal) {
          if ((moving_piece == 'p' || moving_piece == 'P') && SDL_abs(fr - tr) == 2) {
            board->ep_row = (fr + tr) / 2;
            board->ep_col = fc;
          } else {
            board->ep_row = NO_ENPASSANT;
            board->ep_col = NO_ENPASSANT;
          }

          board->promo.captured = board->board[tr][tc];
          board->board[tr][tc]  = moving_piece;
          board->board[fr][fc]  = 0;

          update_castling_rights (board, moving_piece, move);
          update_enpassant_rights(board, moving_piece);

          bool promotion = (tr == 0 || tr == 7) && (moving_piece == 'p' || moving_piece == 'P');

          if (promotion) {
            board->promo.active = true;
            board->promo.row    = tr;
            board->promo.col    = tc;

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