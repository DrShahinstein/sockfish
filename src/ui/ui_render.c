#include "ui.h"
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

void ui_render_init(SDL_Renderer *renderer) {
  SDL_memset(&text_cache, 0, sizeof(TextCache));
  text_cache.renderer         = renderer;
  text_cache.eviction_counter = 0;
}

void ui_render(SDL_Renderer *r, UI_State *ui, EngineWrapper *engine, BoardState *board) {
  // Panel
  SDL_FRect panel = {BOARD_SIZE, 0, UI_WIDTH, BOARD_SIZE};
  SDL_SetRenderDrawColor(r, 40, 44, 52, 255);
  SDL_RenderFillRect(r, &panel);

  // Engine Toggler
  SDL_FRect tog = ui->engine_toggler.rect;
  SDL_SetRenderDrawColor(r, ui->engine_on ? 100 : 200, ui->engine_on ? 200 : 100, 100, 255);
  SDL_RenderFillRect(r, &tog);
  draw_text(r, ui->fonts.roboto16, ui->engine_on ? "Engine: ON" : "Engine: OFF", FWHITE, tog.x + tog.w + 10, tog.y + 5);

  /* --- FEN Input Area --- */
  render_text_input(r, &ui->fen_loader, &ui->fonts);

  SDL_FRect fenbtn = ui->fen_loader.btn.rect;
  SDL_SetRenderDrawColor(r, ui->fen_loader.btn.hovered ? 170 : 200, 200, 200, 255);
  SDL_RenderFillRect(r, &fenbtn);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &fenbtn);
  draw_text_centered(r, ui->fonts.roboto15, "Load Fen", FBLACK, fenbtn);
  
  /* --- PGN Input Area --- */
  render_text_input(r, &ui->pgn_loader, &ui->fonts);

  SDL_FRect pgnbtn = ui->pgn_loader.btn.rect;
  SDL_SetRenderDrawColor(r, ui->pgn_loader.btn.hovered ? 170 : 200, 200, 200, 255);
  SDL_RenderFillRect(r, &pgnbtn);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &pgnbtn);
  draw_text_centered(r, ui->fonts.roboto15, "Load Pgn", FBLACK, pgnbtn);

  // Information Box
  const char *msg = get_info_message();
  draw_text(r, ui->fonts.noto15, "Info:", FWHITE, ui->info_box.rect.x, ui->info_box.rect.y - 20);
  draw_text(r, ui->fonts.noto15, msg, FYELLOW, ui->info_box.rect.x, ui->info_box.rect.y);

  // Turn Changer
  SDL_FRect turn_changer = ui->turn_changer.rect;
  bool w = board->turn == WHITE;
  if (w) SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
  else   SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
  SDL_RenderFillRect(r, &turn_changer);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &turn_changer);
  draw_text(r, ui->fonts.roboto15, w ? "White to play" : "Black to play", FWHITE, turn_changer.x + turn_changer.w + 6, turn_changer.y + 1);

  // Arrow Changer (color)
  SDL_FRect arrow_changer = ui->arrow_changer.rect;
  SDL_FColor ac = ui->arrow_changer.colors[ui->arrow_changer.color_idx];
  SDL_SetRenderDrawColor(r, ac.r*255, ac.g*255, ac.b*255, 255);
  SDL_RenderFillRect(r, &arrow_changer);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &arrow_changer);
  draw_text(r, ui->fonts.roboto15, "Arrow Color", FWHITE, arrow_changer.x + arrow_changer.w + 6, arrow_changer.y + 1);

  // Highlight Changer (color)
  SDL_FRect highlight_changer = ui->hlight_changer.rect;
  SDL_FColor hc = ui->hlight_changer.colors[ui->hlight_changer.color_idx];
  SDL_SetRenderDrawColor(r, hc.r*255, hc.g*255, hc.b*255, 255);
  SDL_RenderFillRect(r, &highlight_changer);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &highlight_changer);
  draw_text(r, ui->fonts.roboto15, "Highlight Color", FWHITE, highlight_changer.x + highlight_changer.w + 6, highlight_changer.y + 1);

  /* --- Sockfish Engine --- */
  if (ui->engine_on) {
    SDL_FRect separator = ui->separator.rect;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderFillRect(r, &separator);
    SDL_RenderRect(r, &separator);

    Move sf_best;
    bool search_thr_active;

    SDL_LockMutex(engine->mtx);
    sf_best = engine->ctx.best;
    search_thr_active = SDL_GetAtomicInt(&engine->thr_working);
    SDL_UnlockMutex(engine->mtx);

    float x = UI_START_X;
    float y = ui->separator.rect.y + 20;

    if (search_thr_active) draw_text(r, ui->fonts.roboto15, "Thinking...", FWHITE, x, y);
    else {
      char from_alg[3], to_alg[3];
      Square sf_from = move_from(sf_best);
      Square sf_to   = move_to(sf_best);
      sq_to_alg(sf_from, from_alg);
      sq_to_alg(sf_to,   to_alg);

      char move_str[32];
      SDL_snprintf(move_str, sizeof(move_str), "Best Move => %s%s", from_alg, to_alg);

      draw_text(r, ui->fonts.roboto15, move_str, FWHITE, x, y);
    }

    engine_req_search(engine, board);
  }

  // Undo Button
  SDL_FRect undobtn = ui->undo_btn.rect;
  SDL_SetRenderDrawColor(r, ui->undo_btn.hovered ? 170 : 200, 200, 200, 255);
  SDL_RenderFillRect(r, &undobtn);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &undobtn);
  draw_text_centered(r, ui->fonts.roboto15, "Undo", FBLACK, ui->undo_btn.rect);

  // Redo Button
  SDL_FRect redobtn = ui->redo_btn.rect;
  SDL_SetRenderDrawColor(r, ui->redo_btn.hovered ? 170 : 200, 200, 200, 255);
  SDL_RenderFillRect(r, &redobtn);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &redobtn);
  draw_text_centered(r, ui->fonts.roboto15, "Redo", FBLACK, ui->redo_btn.rect);

  // Reset Button
  SDL_FRect resetbtn = ui->reset_btn.rect; 
  SDL_SetRenderDrawColor(r, ui->reset_btn.hovered ? 170 : 200, 200, 200, 255);
  SDL_RenderFillRect(r, &resetbtn);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &resetbtn);
  draw_text_centered(r, ui->fonts.roboto15, "Reset", FBLACK, ui->reset_btn.rect);
}

void ui_render_cleanup(void) {
  for (int i = 0; i < TEXT_CACHE_SIZE; ++i) {
    if (text_cache.entries[i].texture) {
      SDL_DestroyTexture(text_cache.entries[i].texture);
      text_cache.entries[i].texture = NULL;
    }
  }
}