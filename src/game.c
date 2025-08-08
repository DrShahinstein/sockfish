#include "game.h"
#include "board.h"
#include "ui.h"
#include <SDL3/SDL.h>

void draw_game(SDL_Renderer *renderer, struct GameState *game_state, UI_State *ui_state) {
  draw_board(renderer, game_state);
  ui_draw(renderer, ui_state);
  SDL_RenderPresent(renderer);
}
