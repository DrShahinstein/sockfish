#include "window.h"
#include "ui.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

static void init_text_input_buffer(UI_TextInput *text_inp, size_t max_len) {
  text_inp->buf    = SDL_malloc(max_len * sizeof(char));
  text_inp->buf[0] = '\0';
}
static void cleanup_text_input_buffer(UI_TextInput *text_inp) {
  SDL_free(text_inp->buf);
}

void ui_init(UI_State *ui) {
  FontMenu fonts = {
    .roboto16 = TTF_OpenFont(ROBOTO, 16),
    .roboto15 = TTF_OpenFont(ROBOTO, 15),
    .jbmono14 = TTF_OpenFont(JBMONO, 14),
  };

  ui->fonts                   = fonts;
  ui->engine_on               = false;
  ui->engine_toggler.rect     = (SDL_FRect){UI_START_X, UI_START_Y, 30, 30};
  ui->engine_toggler.hovered  = false;
  ui->fen_loader.type         = FEN;
  ui->fen_loader.active       = false;
  ui->fen_loader.area.rect    = (SDL_FRect){UI_MIDDLE-115, UI_START_Y+50, UI_FILLER_W, 70};
  ui->fen_loader.area.hovered = false; 
  ui->fen_loader.length       = 0;
  ui->fen_loader.btn.rect     = (SDL_FRect){UI_MIDDLE-50, ui->fen_loader.area.rect.y + 75, 100, 30};
  ui->fen_loader.btn.hovered  = false;
  ui->pgn_loader.type         = PGN;
  ui->pgn_loader.active       = false;
  ui->pgn_loader.area.rect    = (SDL_FRect){UI_MIDDLE-115, ui->fen_loader.btn.rect.y + 40, UI_FILLER_W, 70};
  ui->pgn_loader.area.hovered = false; 
  ui->pgn_loader.length       = 0;
  ui->pgn_loader.btn.rect     = (SDL_FRect){UI_MIDDLE-50, ui->pgn_loader.area.rect.y + 75, 100, 30};
  ui->pgn_loader.btn.hovered  = false;
  ui->separator.rect          = (SDL_FRect){UI_START_X, ui->pgn_loader.btn.rect.y + 50, UI_FILLER_W, 4};
  ui->turn_changer.rect       = (SDL_FRect){UI_START_X, ui->separator.rect.y + 25, 30, 30};
  ui->turn_changer.hovered    = false;
  ui->undo_btn.rect           = (SDL_FRect){UI_MIDDLE-104, BOARD_SIZE-80, 100, 30};
  ui->undo_btn.hovered        = false;
  ui->redo_btn.rect           = (SDL_FRect){UI_MIDDLE+4,   BOARD_SIZE-80, 100, 30};
  ui->redo_btn.hovered        = false;
  ui->reset_btn.rect          = (SDL_FRect){UI_MIDDLE-50,  BOARD_SIZE-40, 100, 30};
  ui->reset_btn.hovered       = false;

  SDL_strlcpy(ui->fen_loader.placeholder, FEN_PLACEHOLDER, sizeof(ui->fen_loader.placeholder));
  init_text_input_buffer(&ui->fen_loader, MAX_FEN);

  SDL_strlcpy(ui->pgn_loader.placeholder, PGN_PLACEHOLDER, sizeof(ui->pgn_loader.placeholder));
  init_text_input_buffer(&ui->pgn_loader, MAX_PGN);

  if (!ui->fonts.roboto16) SDL_Log("Could not load font: %s", SDL_GetError());
  if (!ui->fonts.roboto15) SDL_Log("Could not load font: %s", SDL_GetError());
  if (!ui->fonts.jbmono14) SDL_Log("Could not load font: %s", SDL_GetError());
}

void ui_destroy(UI_State *ui) {
  cleanup_text_input_buffer(&ui->fen_loader);
  cleanup_text_input_buffer(&ui->pgn_loader);

  TTF_CloseFont(ui->fonts.roboto16);
  ui->fonts.roboto16 = NULL;

  TTF_CloseFont(ui->fonts.roboto15);
  ui->fonts.roboto15 = NULL;

  TTF_CloseFont(ui->fonts.jbmono14);
  ui->fonts.jbmono14 = NULL;

  SDL_StopTextInput(SDL_GetKeyboardFocus());
}
