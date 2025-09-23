#pragma once

#include "ui.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define TEXT_CACHE_SIZE 128
#define TEXT_CACHE_MAX_AGE 60000

typedef struct {
  char text[64];
  SDL_Color color;
  TTF_Font *font;
  SDL_Texture *texture;
  float width;
  float height;
  Uint64 last_used;
  bool in_use;
} TextCacheEntry;

typedef struct {
  TextCacheEntry entries[TEXT_CACHE_SIZE];
  SDL_Renderer *renderer;
  int eviction_counter;
} TextCache;

extern TextCache text_cache;

void ui_render_init(SDL_Renderer *renderer);
void ui_render(SDL_Renderer *renderer, UI_State *ui, EngineWrapper *engine, BoardState *board);
void ui_render_cleanup(void);

void evict_old_text_cache(void);
SDL_Texture *get_text_cache(TTF_Font *font, const char *text, SDL_Color color, float *out_width, float *out_height);