#include "window.h"
#include "ui.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

void ui_init(UI_State *ui) {
  ui->font                    = TTF_OpenFont(ROBOTO,16);
  ui->engine_on               = false;
  ui->engine_toggler.rect     = (SDL_FRect){BOARD_SIZE+UI_PADDING,UI_PADDING,30,30};
  ui->engine_toggler.hovered  = false;
  ui->fen_loader.font         = TTF_OpenFont(JBMONO,14);
  ui->fen_loader.area.rect    = (SDL_FRect){BOARD_SIZE+UI_PADDING,UI_PADDING+50,230,90};
  ui->fen_loader.active       = false;
  ui->fen_loader.area.hovered = false; 
  ui->fen_loader.length       = 0;
  ui->fen_loader.input[0]     = '\0';
  ui->fen_loader.btn.rect     = (SDL_FRect){ui->fen_loader.area.rect.x,ui->fen_loader.area.rect.y+100,100,30};
  ui->fen_loader.btn.hovered  = false;
  ui->reset_btn.rect          = (SDL_FRect){ui->fen_loader.area.rect.x+ui->fen_loader.area.rect.w-100,ui->fen_loader.btn.rect.y,100,ui->fen_loader.btn.rect.h};
  ui->reset_btn.hovered       = false;
  ui->separator.rect          = (SDL_FRect){ui->fen_loader.area.rect.x,ui->reset_btn.rect.y+80,ui->fen_loader.area.rect.w,4};
  ui->turn_changer.rect       = (SDL_FRect){ui->fen_loader.area.rect.x,ui->separator.rect.y+25,30,30};
  ui->turn_changer.hovered    = false;

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
