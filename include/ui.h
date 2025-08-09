#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define UI_WIDTH 250
#define UI_PADDING 10
#define ROBOTO "assets/Roboto-Regular.ttf"
#define MAX_FEN 128

typedef struct {
  SDL_FRect rect;
  bool hovered;
  bool active;
} UI_Element;

typedef struct {
  UI_Element area;
  UI_Element btn;
  char input[MAX_FEN];
  size_t length;
} UI_FenLoader;

typedef struct {
  bool engine_on;
  UI_Element toggle_btn;
  UI_FenLoader fen_loader;
  TTF_Font *font;
} UI_State;

void ui_init(UI_State *ui);
void ui_handle_event(UI_State *ui, SDL_Event *e);
void ui_draw(SDL_Renderer *renderer, UI_State *ui);
void ui_destroy(UI_State *ui);
