#pragma once

#include "ui.h"
#include <SDL3/SDL.h>

void ui_render_init(SDL_Renderer *renderer);
void ui_render(SDL_Renderer *renderer, UI_State *ui, EngineWrapper *engine, BoardState *board);
void ui_render_cleanup(void);