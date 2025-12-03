#include "board.h"
#include "window.h"
#include "cursor.h"            /* get_mouse_pos */
#include "sockfish/sockfish.h" /* Move, Turn, CASTLE_WK, CASTLE_WQ, CASTLE_BK, CASTLE_BQ, '= Move Utilities =' ... */
#include <SDL3/SDL.h>

static inline bool is_mouse_in_board(float mx, float my);
static bool check_valid(BoardState *b, Move move);

void board_handle_event(SDL_Event *e, BoardState *board) {
  float mx; float my; int sq_row; int sq_col;

  switch (e->type) {
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    board->selected_piece.active = false;

    if (e->button.button == SDL_BUTTON_LEFT) {
      get_mouse_pos(&mx, &my);

      if (!is_mouse_in_board(mx, my)) {
        board->drag.active = false;
        return;
      }

      clear_annotations(&board->annotations);

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
        board->drag.to_row   = sq_row;
        board->drag.to_col   = sq_col;
        board->drag.from_row = sq_row;
        board->drag.from_col = sq_col;

        board->selected_piece.active = true;
        board->selected_piece.row    = sq_row;
        board->selected_piece.col    = sq_col;

        board_update_valid_moves(board);
      }
    }

    else if (e->button.button == SDL_BUTTON_RIGHT) {
      get_mouse_pos(&mx, &my);

      if (!is_mouse_in_board(mx, my)) return;

      sq_row       = my / SQ;
      sq_col       = mx / SQ;
      Square start = rowcol_to_sq(sq_row, sq_col);

      start_drawing_arrow(&board->annotations, start);
    }

    break;

  case SDL_EVENT_MOUSE_BUTTON_UP:
    if (e->button.button == SDL_BUTTON_RIGHT) {
      get_mouse_pos(&mx, &my);

      if (!is_mouse_in_board(mx, my)) {
        cancel_drawing_arrow(&board->annotations);
        return;
      }

      sq_row     = my / SQ;
      sq_col     = mx / SQ;
      Square end = rowcol_to_sq(sq_row, sq_col);

      if (is_drawing_arrow(&board->annotations)) {
        Square start = get_arrow_start(&board->annotations);

        if (start == end) {
          if (has_highlight(&board->annotations, start)) remove_highlight(&board->annotations, start);
          else add_highlight(&board->annotations, start, board->annotations.highlight_color);
        }
        else {
          if (has_arrow(&board->annotations, start, end)) remove_arrow(&board->annotations, start, end);
          else add_arrow(&board->annotations, start, end, board->annotations.arrow_color);
        }

        cancel_drawing_arrow(&board->annotations);
      }

      return;
    }

    if (e->button.button == SDL_BUTTON_LEFT) {
      get_mouse_pos(&mx, &my);

      if (!is_mouse_in_board(mx, my)) {
        board->drag.active = false;
        return;
      }

      /* -- Handle Promotion -- */
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
          board->board[board->promo.row][board->promo.col]         = board->promo.captured;
          return;
        }

        board->should_update_valid_moves = true;

        char piece = board->promo.choices[p];

        board->board[board->promo.row][board->promo.col] = piece;
        board->turn = (board->turn == WHITE) ? BLACK : WHITE;
        board->promo.active = false;
        board->drag.active  = false;

        // help history to follow up promoted piece
        if (board->undo_count > 0) {
          board->history[board->undo_count - 1].promoted_piece = piece;
        }

        return;
      }

      if (board->drag.active) {
        if (board->promo.active) {
          board->drag.active = false;
          return;
        }

        /* -- Handle Moving -- */
        int fr            = board->drag.from_row;  // from row
        int fc            = board->drag.from_col;  // from col
        int tr            = my / SQ;               // to row
        int tc            = mx / SQ;               // to col
        Square from_sq    = rowcol_to_sq(fr, fc);
        Square to_sq      = rowcol_to_sq(tr, tc);
        Move move         = create_move(from_sq, to_sq);
        char moving_piece = board->board[fr][fc];

        bool valid = check_valid(board, move);

        if (valid) {
          board->should_update_valid_moves = true;
          
          if (board->undo_count < MAX_HISTORY) {
            board->redo_count = 0;
            board_save_history(board, fr, fc, tr, tc, board->undo_count);
            board->undo_count += 1;
          }

          if (is_castling_move(board, move)) {
            perform_castling(board, move);

            board->turn                  = (board->turn == WHITE) ? BLACK : WHITE;
            board->selected_piece.active = false;
            board->drag.active           = false;

            board_update_king_in_check(board);
            
            return;
          }

          else if (is_en_passant_capture(board, move)) {
            int captured_row = board->turn == WHITE ? (tr + 1) : (tr - 1);
            board->board[captured_row][tc] = 0;
            board->board[tr][tc]           = moving_piece;
            board->board[fr][fc]           = 0;
            board->ep_row                  = NO_ENPASSANT;
            board->ep_col                  = NO_ENPASSANT;
            board->turn                    = (board->turn == WHITE) ? BLACK : WHITE;
            board->drag.active             = false;

            board_update_king_in_check(board);

            return;
          }

          // normal move
          else {
            bool double_pawn_push = (moving_piece == 'p' || moving_piece == 'P') && SDL_abs(fr - tr) == 2;

            if (double_pawn_push) {
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
              board->promo.choices[0] = w ? 'Q' : 'q';
              board->promo.choices[1] = w ? 'R' : 'r';
              board->promo.choices[2] = w ? 'N' : 'n';
              board->promo.choices[3] = w ? 'B' : 'b';
            }

            else {
              if (board->turn == WHITE) board->turn = BLACK;
              else                      board->turn = WHITE;
            }
          }

          board_update_king_in_check(board);
        }

        board->drag.active = false;
      }
    }
    break;
  }
}

static inline bool is_mouse_in_board(float mx, float my) {
  return mx >= 0 && mx < BOARD_SIZE && my >= 0 && my < BOARD_SIZE;
}

static bool check_valid(BoardState *b, Move move) {
  if (b->valid_moves.count == 0) return false;

  for (int i = 0; i < b->valid_moves.count; ++i) {
    Move m_valid = b->valid_moves.moves[i];

    // this dodges move type differences (MOVE_NORMAl, MOVE_CASTLING...)
    bool same_origin_and_destination = move_from(move) == move_from(m_valid) &&
                                       move_to(move)   == move_to(m_valid);
    
    if (same_origin_and_destination) 
      return true;
  }

  return false;
}