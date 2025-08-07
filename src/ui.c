#include "ui.h"
#include "window.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

void ui_init(UI_State *ui) {
  ui->engine_on = true;
  strcpy(ui->fen_buffer, "");
  ui->reset_button.rect = (SDL_FRect){820, 50, 200, 30};
  ui->loadfen_button.rect = (SDL_FRect){820, 200, 200, 30};
  ui->fen_box = (SDL_FRect){820, 90, 200, 100};
}

void ui_draw(SDL_Renderer *r, UI_State *ui) {
  (void)ui;
  SDL_FRect panel = {BOARD_SIZE, 0, BOARD_SIZE, BOARD_SIZE};
  SDL_SetRenderDrawColor(r, 40, 44, 52, 255);
  SDL_RenderFillRect(r, &panel);
  // codecodecodecodecode
}

void ui_handle_event(UI_State *ui, SDL_Event *e) {
  (void)ui; (void)e;
  // codecodecodecodecode
}
