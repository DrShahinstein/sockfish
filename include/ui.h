#pragma once

#include "engine.h"
#include "board.h"
#include "cursor.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define UI_WIDTH 250
#define UI_PADDING 10
#define ROBOTO "assets/Roboto-Regular.ttf"
#define JBMONO "assets/JetBrainsMonoNL-Regular.ttf" 
#define MAX_FEN 128
#define FEN_PLACEHOLDER "Enter FEN here..."
#define FWHITE (SDL_Color){255,255,255,255}
#define FBLACK (SDL_Color){0,0,0,255}
#define FGRAY  (SDL_Color){150,150,150,255}

typedef struct {
  SDL_FRect rect;
  bool hovered;
} UI_Element;

typedef struct {
  TTF_Font *font;
  bool active;
  UI_Element area;
  UI_Element btn;
  char input[MAX_FEN];
  size_t length;
} UI_FenLoader;

typedef struct {
  bool engine_on;
  UI_Element engine_toggler;
  UI_Element undo_btn;
  UI_Element turn_changer;
  UI_Element reset_btn;
  UI_FenLoader fen_loader;
  UI_Element separator;
  TTF_Font *font;
} UI_State;

void ui_init(UI_State *ui);
void ui_handle_event(SDL_Event *e, UI_State *ui, BoardState *board);
void ui_render(SDL_Renderer *renderer, UI_State *ui, EngineWrapper *engine, BoardState *board);
void ui_destroy(UI_State *ui);
