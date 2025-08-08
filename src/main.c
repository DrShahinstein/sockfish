#include "window.h"
#include "game.h"
#include "board.h"
#include "event.h"
#include "ui.h"
#include <SDL3/SDL.h>

int main(int argc, char *argv[]) {
  (void)argc; (void)argv;

  SDL_Init(SDL_INIT_VIDEO);

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
  struct GameState game = {
    .running = true,
  };

  initialize_board(renderer, &game);
  ui_init(&ui);

  while (game.running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      handle_event(&e, &game);
      ui_handle_event(&ui, &e);
    }
    draw_game(renderer, &game, &ui);
  }

  cleanup_textures(&game);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
