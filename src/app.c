#include "app.h"
#include "ui.h"
#include "ui_render.h"
#include "cursor.h"
#include "board.h"
#include "board_render.h"
#include "engine.h"
#include "sockfish/config.h"
#include "sockfish/uci.h"
#include "sockfish/transposition_table.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

static const float SCALE_FACTOR=1.0f;
static const U64 CONFIG_CHECK_INTERVAL_MS=1000;

static void check_config(AppState *app);

void sdl(void) {
  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();
  init_cursors();

  AppState app = {0};

  app.last_ini_mtime = config_get_modification_time(SOCKFISH_INI);
  app.last_ini_check = SDL_GetTicks();

  U64 prev                    = SDL_GetPerformanceCounter();
  double freq                 = (double)SDL_GetPerformanceFrequency();
  const double target_ms      = 1000.0 / 60.0; // 60 FPS
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

  app.window = SDL_CreateWindow(W_TITLE, w_width, w_height, SDL_WINDOW_HIGH_PIXEL_DENSITY);
  if (app.window == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
    goto quit;
  }

  app.renderer = SDL_CreateRenderer(app.window, NULL);
  if (app.renderer == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create renderer: %s\n", SDL_GetError());
    goto quit;
  }

  g_renderer = app.renderer;

  SDL_SetRenderLogicalPresentation(app.renderer, W_WIDTH, W_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

  engine_init(&app.engine);
  board_init(&app.board);
  render_board_init(app.renderer);
  ui_init(&app.ui);
  ui_render_init(app.renderer);

  app.running = true;

  while (app.running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT) app.running = false;

      SDL_ConvertEventToRenderCoordinates(app.renderer, &e);

      board_handle_event(&e, &app.board);
      ui_handle_event(&e, &app.ui, &app.board, &app.engine);
    }

    if (app.ui.engine_on) {
      engine_req_search(&app.engine, &app.board);
    }

    check_config(&app);

    render_board(app.renderer, &app.board);
    ui_render(app.renderer, &app.ui, &app.engine, &app.board);
    SDL_RenderPresent(app.renderer);

    U64 now           = SDL_GetPerformanceCounter();
    double elapsed_ms = (now - prev) * 1000.0 / freq;
    if (elapsed_ms < target_ms) SDL_Delay((Uint32)(target_ms - elapsed_ms));
    prev = SDL_GetPerformanceCounter();
  }

quit:
  ui_destroy(&app.ui);
  cleanup_cursors();
  engine_destroy(&app.engine);
  render_board_cleanup();
  ui_render_cleanup();
  SDL_DestroyRenderer(app.renderer);
  SDL_DestroyWindow(app.window);
  TTF_Quit();
  SDL_Quit();
}

void uci(void) {
  uci_loop(); // sockfish/uci.c
}


static void check_config(AppState *app) {
  U64 current_ticks = SDL_GetTicks();

  if (current_ticks - app->last_ini_check > CONFIG_CHECK_INTERVAL_MS) {
    app->last_ini_check = current_ticks;
    U64 current_mtime   = config_get_modification_time(SOCKFISH_INI);
      
    if (current_mtime != app->last_ini_mtime && current_mtime != 0) {
      app->last_ini_mtime = current_mtime;
      
      SF_Config updated_config;
      config_load(SOCKFISH_INI, &updated_config);
      engine_update_config(&app->engine, &updated_config);
    }
  }
}

