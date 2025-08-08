#include "event.h"
#include "board.h"
#include "game.h"
#include <SDL3/SDL.h>

bool is_mouse_in_board(float mx, float my);

void handle_event(SDL_Event *e, GameState *game) {
  float mx; float my; int sq_row; int sq_col;

  switch (e->type) {
  case SDL_EVENT_QUIT:
    game->running = false;
    break;
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    if (e->button.button == SDL_BUTTON_LEFT) {
      SDL_GetMouseState(&mx, &my);

      if (!is_mouse_in_board(mx, my)) {
        game->drag.active = false;
        return;
      }

      sq_col = mx / SQ;
      sq_row = my / SQ;
      char piece = game->board[sq_row][sq_col];

      if (piece != 0) {
        game->drag.active = true;
        game->drag.row = sq_row;
        game->drag.col = sq_col;
        game->drag.from_row = sq_row;
        game->drag.from_col = sq_col;
      }
    }
    break;
  case SDL_EVENT_MOUSE_BUTTON_UP:
    if (e->button.button == SDL_BUTTON_LEFT) {
      SDL_GetMouseState(&mx, &my);

      if (!is_mouse_in_board(mx, my)) {
        game->drag.active = false;
        return;
      }

      sq_col = mx / SQ;
      sq_row = my / SQ;

      if (game->drag.active) {
        int from_r = game->drag.from_row;
        int from_c = game->drag.from_col;
        char moving_piece = game->board[from_r][from_c];
        bool legal = true; // for now
        bool dropped_same = from_r == sq_row && from_c == sq_col;
        
        if (dropped_same) {
          game->drag.active = false;
          return;
        }

        if (legal) {
          game->board[sq_row][sq_col] = moving_piece;
          game->board[from_r][from_c] = 0;
        }

        game->drag.active = false;
      }
    }
    break;
  }
}

bool is_mouse_in_board(float mx, float my) {
  return mx >= 0 && mx < BOARD_SIZE && my >= 0 && my < BOARD_SIZE; 
}
