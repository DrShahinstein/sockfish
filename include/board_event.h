#pragma once

#include "board.h"
#include "ui.h" // UI_State
#include <SDL3/SDL.h>

void board_handle_event(SDL_Event *e, BoardState *board, UI_State *ui);
