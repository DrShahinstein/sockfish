#include "window.h"
#include "ui.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

static void init_text_input_buffers(UI_TextInput *text_inp, size_t max_len);
static void cleanup_text_input_buffers(UI_TextInput *text_inp);

static SDL_FColor ARROW_COLORS[] = {
  DEFAULT_ARROW_COLOR,                                      // red
  {32.0f/255.0f,     129.0f/255.0f,  28.0f/255.0f,  0.8f},  // green
  {13.0f/255.0f,     110.0f/255.0f,  253.0f/255.0f, 0.8f},  // blue
  {230.0f/255.0f,    42.0f/255.0f,  163.0f/255.0f,  0.8f}   // cerise :O
};

void ui_init(UI_State *ui) {
  FontMenu fonts = {
    .roboto16 = TTF_OpenFont(ROBOTO, 16),
    .roboto15 = TTF_OpenFont(ROBOTO, 15),
    .noto15   = TTF_OpenFont(NOTO,   15),
    .jbmono14 = TTF_OpenFont(JBMONO, 14),
  };

  info_system_init();

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
  ui->info_box.rect           = (SDL_FRect){UI_START_X, ui->pgn_loader.btn.rect.y + 70, UI_FILLER_W, 25};
  ui->info_box.hovered        = false;
  ui->turn_changer.rect       = (SDL_FRect){UI_START_X, ui->info_box.rect.y + 36, 20, 20};
  ui->turn_changer.hovered    = false;
  ui->arrow_changer.rect      = (SDL_FRect){UI_START_X, ui->turn_changer.rect.y + 25, 20, 20};
  ui->arrow_changer.hovered   = false;
  ui->arrow_changer.colors    = ARROW_COLORS;
  ui->arrow_changer.color_idx = 0;
  ui->separator.rect          = (SDL_FRect){UI_START_X, ui->arrow_changer.rect.y + 40, UI_FILLER_W, 2};
  ui->undo_btn.rect           = (SDL_FRect){UI_MIDDLE-104, BOARD_SIZE-80, 100, 30};
  ui->undo_btn.hovered        = false;
  ui->redo_btn.rect           = (SDL_FRect){UI_MIDDLE+4,   BOARD_SIZE-80, 100, 30};
  ui->redo_btn.hovered        = false;
  ui->reset_btn.rect          = (SDL_FRect){UI_MIDDLE-50,  BOARD_SIZE-40, 100, 30};
  ui->reset_btn.hovered       = false;

  SDL_strlcpy(ui->fen_loader.placeholder, FEN_PLACEHOLDER, sizeof(ui->fen_loader.placeholder));
  init_text_input_buffers(&ui->fen_loader, MAX_FEN);

  SDL_strlcpy(ui->pgn_loader.placeholder, PGN_PLACEHOLDER, sizeof(ui->pgn_loader.placeholder));
  init_text_input_buffers(&ui->pgn_loader, MAX_PGN);

  if (!ui->fonts.roboto16) SDL_Log("Could not load font: %s", SDL_GetError());
  if (!ui->fonts.roboto15) SDL_Log("Could not load font: %s", SDL_GetError());
  if (!ui->fonts.jbmono14) SDL_Log("Could not load font: %s", SDL_GetError());
  if (!ui->fonts.noto15)   SDL_Log("Could not load font: %s", SDL_GetError());
}

void ui_destroy(UI_State *ui) {
  cleanup_text_input_buffers(&ui->fen_loader);
  cleanup_text_input_buffers(&ui->pgn_loader);

  TTF_CloseFont(ui->fonts.roboto16);
  ui->fonts.roboto16 = NULL;

  TTF_CloseFont(ui->fonts.roboto15);
  ui->fonts.roboto15 = NULL;

  TTF_CloseFont(ui->fonts.jbmono14);
  ui->fonts.jbmono14 = NULL;

  TTF_CloseFont(ui->fonts.noto15);
  ui->fonts.noto15 = NULL;

  SDL_StopTextInput(SDL_GetKeyboardFocus());
}

static void init_text_input_buffers(UI_TextInput *text_inp, size_t max_len) {
  text_inp->buf               = SDL_malloc(max_len * sizeof(char));
  text_inp->buf[0]            = '\0';
  text_inp->cached_line_count = 0;
  text_inp->cached_text_hash  = 0;
  text_inp->cached_wrap_width = 0;
  text_inp->cache_valid       = false;

  int max_lines    = (text_inp->type == FEN) ? 3 : 10;
  int max_buf_size = (text_inp->type == FEN) ? MAX_FEN : MAX_PGN;

  text_inp->wrap_lines      = SDL_malloc(max_lines * sizeof(char *));
  text_inp->wrap_line_count = max_lines;

  for (int i = 0; i < max_lines; ++i) {
    text_inp->wrap_lines[i]    = SDL_malloc(max_buf_size);
    text_inp->wrap_lines[i][0] = '\0';
  }
}

static void cleanup_text_input_buffers(UI_TextInput *text_inp) {
  SDL_free(text_inp->buf);

  if (text_inp->wrap_lines) {
    for (int i = 0; i < text_inp->wrap_line_count; ++i) {
      SDL_free(text_inp->wrap_lines[i]);
    }

    SDL_free(text_inp->wrap_lines);
    text_inp->wrap_lines      = NULL;
    text_inp->wrap_line_count = 0;
  }
}