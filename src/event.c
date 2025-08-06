#include "event.h"
#include "board.h"
#include "game.h"
#include "SDL3/SDL.h"

void handle_event(SDL_Event *e, struct GameState *state) {
  float mx; float my; int sq_row; int sq_col;

  switch (e->type) {
  case SDL_EVENT_QUIT:
    state->running = false;
    break;
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    if (e->button.button == SDL_BUTTON_LEFT) {
      SDL_GetMouseState(&mx, &my);

      sq_col = mx / SQ;
      sq_row = my / SQ;
      char piece = state->board[sq_row][sq_col];

      if (piece != 0) {
        state->drag.active = true;
        state->drag.row = sq_row;
        state->drag.col = sq_col;
        state->drag.from_row = sq_row;
        state->drag.from_col = sq_col;
      }
    }
    break;
  case SDL_EVENT_MOUSE_BUTTON_UP:
    if (e->button.button == SDL_BUTTON_LEFT) {
      SDL_GetMouseState(&mx, &my);
      sq_col = mx / SQ;
      sq_row = my / SQ;

      if (state->drag.active) {
        int from_r = state->drag.from_row;
        int from_c = state->drag.from_col;
        char moving_piece = state->board[from_r][from_c];
        bool legal = true; // for now
        bool dropped_same = from_r == sq_row && from_c == sq_col;
        
        if (dropped_same) {
          state->drag.active = false;
          return;
        }

        if (legal) {
          state->board[sq_row][sq_col] = moving_piece;
          state->board[from_r][from_c] = 0;
        }

        state->drag.active = false;
      }
    }
    break;
  }
}
