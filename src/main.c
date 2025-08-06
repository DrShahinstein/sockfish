#include "window.h"
#include "game.h"
#include "board.h"
#include "event.h"
#include <SDL3/SDL.h>

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  SDL_Init(SDL_INIT_VIDEO);

  SDL_Window *window = SDL_CreateWindow(W_TITLE, W_SIZE, W_SIZE, 0);
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
  
  struct GameState state = {
    .running = true,
  };

  initialize_board(renderer, &state);

  while (state.running) {
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
      handle_event(&e, &state);
    }
    
    draw_game(renderer, &state);
  }

  cleanup_textures(&state);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
