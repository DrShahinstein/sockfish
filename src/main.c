#include "window.h"
#include "game.h"
#include "board.h"
#include "event.h"
#include "ui.h"
#include "sockfish.h"
#include "cursor.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdint.h>

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
  GameState game = {
    .running = true,
    .engine  = NULL,
  };

  game.engine = sf_create();
  if (!game.engine) SDL_Log("Sockfish could not load");

  initialize_board(renderer, &game);
  ui_init(&ui);

  while (game.running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      handle_event(&e, &game);
      ui_handle_event(&e, &ui, &game);
    }
    
    draw_game(renderer, &game, &ui);

    uint64_t now = SDL_GetPerformanceCounter();
    double elapsed_ms = (now - prev) * 1000.0 / freq;
    if (elapsed_ms < target_ms) SDL_Delay((Uint32)(target_ms - elapsed_ms));
    prev = SDL_GetPerformanceCounter();
  }
  
  cleanup_cursors();
  ui_destroy(&ui);
  TTF_Quit();
  sf_destroy(game.engine);
  cleanup_textures(&game);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
