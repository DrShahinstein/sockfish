#include "window.h"
#include "ui.h"
#include "ui_render.h"
#include "cursor.h"
#include "board.h"
#include "board_render.h"
#include "engine.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define SCALE_FACTOR 1.0f

int main(int argc, char *argv[]) {
  (void)argc; (void)argv;

  uint64_t prev          = SDL_GetPerformanceCounter();
  double freq            = (double)SDL_GetPerformanceFrequency();
  const double target_ms = 1000.0 / 60.0; // 60 FPS

  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();
  init_cursors();

  SDL_DisplayID display_id    = SDL_GetPrimaryDisplay();
  const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display_id);

  float scale = SCALE_FACTOR;
  if (mode) {
    int display_width = mode->w;

    if      (display_width >= 3840) scale = 2.0f;
    else if (display_width >= 2560) scale = 1.5f;
    else if (display_width >= 1920) scale = 1.0f;
    else                            scale = 0.9f;
  }
  else {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Could not get display mode: %s\n", SDL_GetError());
  }

  int w_width  = (int)(W_WIDTH  * scale);
  int w_height = (int)(W_HEIGHT * scale);

  SDL_Window *window = SDL_CreateWindow(W_TITLE, w_width, w_height, SDL_WINDOW_HIGH_PIXEL_DENSITY);
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
  g_renderer = renderer;
  
  SDL_SetRenderLogicalPresentation(renderer, W_WIDTH, W_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

  UI_State      ui={0};
  EngineWrapper engine={0}; 
  BoardState    board={0};

  board_init(&board);      /**/    render_board_init(renderer);
  engine_init(&engine);
  ui_init(&ui);            /**/    ui_render_init(renderer);

  bool running = true;
  while (running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT) running = false;

      SDL_ConvertEventToRenderCoordinates(renderer, &e);

      board_handle_event(&e, &board);
      ui_handle_event(&e, &ui, &board);
    }

    render_board(renderer, &board);
    ui_render(renderer, &ui, &engine, &board);
    SDL_RenderPresent(renderer);

    uint64_t now      = SDL_GetPerformanceCounter();
    double elapsed_ms = (now - prev) * 1000.0 / freq;
    if (elapsed_ms < target_ms) SDL_Delay((Uint32)(target_ms - elapsed_ms));
    prev = SDL_GetPerformanceCounter();
  }
  
  ui_destroy(&ui);
  cleanup_cursors();
  engine_destroy(&engine);
  render_board_cleanup();
  ui_render_cleanup();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  TTF_Quit();
  SDL_Quit();

  return 0;
}
