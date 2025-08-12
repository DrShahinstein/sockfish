#pragma once

#include "board.h"
#include <SDL3/SDL.h>

void board_handle_event(SDL_Event *e, BoardState *board);
