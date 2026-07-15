#pragma once

#include "board.h"
#include "ui.h"
#include "engine.h"

#define W_WIDTH (BOARD_SIZE + UI_WIDTH)
#define W_HEIGHT BOARD_SIZE
#define W_TITLE "Sockfish"

typedef struct {
  UI_State ui;
  EngineWrapper engine;
  BoardState board;
  U64 last_ini_check;
  U64 last_ini_mtime;
  SDL_Window *window;
  SDL_Renderer *renderer;
  bool running;
} AppState;

void sdl(void);
void uci(void);

