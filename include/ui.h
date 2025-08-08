#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define UI_WIDTH 250
#define UI_PADDING 10
#define ROBOTO "assets/Roboto-Regular.ttf"

typedef struct {
  SDL_FRect rect;
  bool hovered;
  bool active;
} UI_Button;

typedef struct {
  bool engine_on;
  UI_Button toggle_button;
  TTF_Font *font;
} UI_State;

void ui_init(UI_State *ui);
void ui_handle_event(UI_State *ui, SDL_Event *e);
void ui_draw(SDL_Renderer *renderer, UI_State *ui);
void ui_destroy(UI_State *ui);
