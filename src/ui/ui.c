#include "window.h"
#include "ui.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

void ui_init(UI_State *ui) {
  ui->font                    = TTF_OpenFont(ROBOTO,16);
  ui->engine_on               = false;
  ui->engine_toggler.rect     = (SDL_FRect){UI_START_X, UI_START_Y, 30, 30};
  ui->engine_toggler.hovered  = false;
  ui->fen_loader.font         = TTF_OpenFont(JBMONO,14);
  ui->fen_loader.area.rect    = (SDL_FRect){UI_MIDDLE-115, UI_START_Y+50, UI_FILLER_W, 90};
  ui->fen_loader.active       = false;
  ui->fen_loader.area.hovered = false; 
  ui->fen_loader.length       = 0;
  ui->fen_loader.input[0]     = '\0';
  ui->fen_loader.btn.rect     = (SDL_FRect){UI_MIDDLE-50, ui->fen_loader.area.rect.y + 95, 100, 30};
  ui->fen_loader.btn.hovered  = false;
  ui->separator.rect          = (SDL_FRect){UI_START_X, ui->fen_loader.btn.rect.y + 50, UI_FILLER_W, 4};
  ui->turn_changer.rect       = (SDL_FRect){UI_START_X, ui->separator.rect.y + 25, 30, 30};
  ui->turn_changer.hovered    = false;
  ui->undo_btn.rect           = (SDL_FRect){UI_MIDDLE-104, BOARD_SIZE-80, 100, 30};
  ui->undo_btn.hovered        = false;
  ui->redo_btn.rect           = (SDL_FRect){UI_MIDDLE+4,   BOARD_SIZE-80, 100, 30};
  ui->redo_btn.hovered        = false;
  ui->reset_btn.rect          = (SDL_FRect){UI_MIDDLE-50,  BOARD_SIZE-40, 100, 30};
  ui->reset_btn.hovered       = false;

  if (!ui->font)            SDL_Log("Could not load font: %s", SDL_GetError());
  if (!ui->fen_loader.font) SDL_Log("Could not load font: %s", SDL_GetError());
}

void ui_destroy(UI_State *ui) {
  if (ui->font) {
    TTF_CloseFont(ui->font);
    ui->font = NULL;
  }

  if (ui->fen_loader.font) {
    TTF_CloseFont(ui->fen_loader.font);
    ui->fen_loader.font = NULL;
  }

  SDL_StopTextInput(SDL_GetKeyboardFocus());
}
