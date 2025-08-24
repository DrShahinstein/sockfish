#include "window.h"
#include "ui.h"
#include "cursor.h"
#include "board.h"
#include "board_render.h"
#include "board_event.h"
#include "engine.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

int main(int argc, char *argv[]) {
  (void)argc; (void)argv;

  uint64_t prev = SDL_GetPerformanceCounter();
  double freq = (double)SDL_GetPerformanceFrequency();
  const double target_ms = 1000.0 / 60.0; // 60 FPS

  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();
  init_cursors();

  SDL_Window *window = SDL_CreateWindow(W_TITLE, W_WIDTH, W_HEIGHT, 0);
  if (window == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
  if (renderer == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create renderer: %s\n", SDL_GetError());
    return 1;
  }
  
  UI_State ui;
  EngineWrapper engine; 
  BoardState board = {
    .running = true,
  };

  board_init(&board); render_board_init(renderer, &board);
  engine_init(&engine);
  ui_init(&ui);

  while (board.running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      board_handle_event(&e, &board);
      ui_handle_event(&e, &ui, &board);
    }
    
    render_board(renderer, &board);
    ui_render(renderer, &ui, &engine, &board);
    SDL_RenderPresent(renderer);

    uint64_t now = SDL_GetPerformanceCounter();
    double elapsed_ms = (now - prev) * 1000.0 / freq;
    if (elapsed_ms < target_ms) SDL_Delay((Uint32)(target_ms - elapsed_ms));
    prev = SDL_GetPerformanceCounter();
  }
  
  cleanup_cursors();
  ui_destroy(&ui);
  engine_destroy(&engine);
  render_board_cleanup(renderer, &board);
  TTF_Quit();
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
