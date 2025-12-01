#include "cursor.h"

SDL_Cursor *cursor_default = NULL;
SDL_Cursor *cursor_pointer = NULL;
SDL_Cursor *cursor_text    = NULL;
SDL_Renderer *g_renderer   = NULL;

void init_cursors(void) {
  cursor_default = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
  cursor_pointer = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
  cursor_text    = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
}

void cleanup_cursors(void) {
  if (cursor_pointer) SDL_DestroyCursor(cursor_pointer);
  if (cursor_default) SDL_DestroyCursor(cursor_default);
  if (cursor_text)    SDL_DestroyCursor(cursor_text);
}

inline bool cursor_in_rect(float x, float y, SDL_FRect *r) {
  return x >= r->x && x < (r->x + r->w) && y >= r->y && y < (r->y + r->h);
}

void get_mouse_pos(float *x, float *y)  {
  float window_x, window_y;
  SDL_GetMouseState(&window_x, &window_y);

  if (g_renderer) {
    SDL_RenderCoordinatesFromWindow(g_renderer, window_x, window_y, x, y);
  } else {
    *x = window_x;
    *y = window_y;
  }
}