#pragma once

#include <SDL3/SDL.h>

#define UI_WIDTH 250

typedef struct {
  SDL_FRect rect;
  bool hovered;
  bool active;
} UI_Widget;

typedef struct {
  UI_Widget reset_button;
  UI_Widget loadfen_button;
  SDL_FRect fen_box;
  char fen_buffer[128];
  bool engine_on;
} UI_State;

void ui_init(UI_State *ui);
void ui_handle_event(UI_State *ui, SDL_Event *e);
void ui_draw(SDL_Renderer *renderer, UI_State *ui);
