#include "ui_render.h"
#include <SDL3/SDL.h>

TextCache text_cache;

void evict_old_text_cache(void) {
  Uint64 current_time = SDL_GetTicks();

  for (int i = 0; i < TEXT_CACHE_SIZE; ++i) {
    TextCacheEntry *entry = &text_cache.entries[i];

    if (!entry->in_use)
      continue;

    if (current_time - entry->last_used > TEXT_CACHE_MAX_AGE) {
      if (entry->texture) {
        SDL_DestroyTexture(entry->texture);
      }
      entry->in_use = false;
    }
  }
}

SDL_Texture *get_text_cache(TTF_Font *font, const char *text, SDL_Color color, float *out_width, float *out_height) {
  if (!text || !font || SDL_strlen(text) == 0) {
    if (out_width)  *out_width  = 0;
    if (out_height) *out_height = 0;
    return NULL;
  }

  Uint64 current_time = SDL_GetTicks();

  for (int i = 0; i < TEXT_CACHE_SIZE; ++i) {
    TextCacheEntry *entry = &text_cache.entries[i];

    if (!entry->in_use)
      continue;

    bool is_cache_hit =
        entry->font    == font    && SDL_strcmp(entry->text, text) == 0 &&
        entry->color.r == color.r && entry->color.g == color.g          &&
        entry->color.b == color.b && entry->color.a == color.a;

    if (is_cache_hit) {
      entry->last_used            = current_time;
      if (out_width) *out_width   = entry->width;
      if (out_height) *out_height = entry->height;
      return entry->texture;
    }
  }

  if (++text_cache.eviction_counter > 100) {
    text_cache.eviction_counter = 0;
    evict_old_text_cache();
  }

  int slot           = -1;
  Uint64 oldest_time = UINT64_MAX;
  int oldest_slot    = 0;

  for (int i = 0; i < TEXT_CACHE_SIZE; ++i) {
    if (!text_cache.entries[i].in_use) {
      slot = i;
      break;
    }

    if (text_cache.entries[i].last_used < oldest_time) {
      oldest_time = text_cache.entries[i].last_used;
      oldest_slot = i;
    }
  }

  if (slot == -1) {
    slot = oldest_slot;
    if (text_cache.entries[slot].texture) {
      SDL_DestroyTexture(text_cache.entries[slot].texture);
    }
  }

  TextCacheEntry *entry = &text_cache.entries[slot];

  SDL_Surface *surf = TTF_RenderText_Blended(font, text, 0, color);
  if (!surf) {
    if (out_width)  *out_width  = 0;
    if (out_height) *out_height = 0;
    return NULL;
  }

  SDL_Texture *tex = SDL_CreateTextureFromSurface(text_cache.renderer, surf);

  SDL_strlcpy(entry->text, text, sizeof(entry->text));
  entry->color     = color;
  entry->font      = font;
  entry->texture   = tex;
  entry->width     = (float)surf->w;
  entry->height    = (float)surf->h;
  entry->last_used = current_time;
  entry->in_use    = true;

  SDL_DestroySurface(surf);

  if (out_width)  *out_width  = entry->width;
  if (out_height) *out_height = entry->height;

  return tex;
}
