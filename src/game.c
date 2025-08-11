#include "game.h"
#include "board.h"
#include "ui.h"
#include "sockfish.h"
#include <SDL3/SDL.h>

void draw_game(SDL_Renderer *renderer, GameState *game, UI_State *ui, Sockfish *sockfish) {
  draw_board(renderer, game);
  ui_draw(renderer, ui, sockfish);
  SDL_RenderPresent(renderer);
}
