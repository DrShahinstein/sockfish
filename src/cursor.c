#include "cursor.h"

SDL_Cursor *cursor_default = NULL;
SDL_Cursor *cursor_pointer = NULL;

void init_cursors(void) {
  cursor_default = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
  cursor_pointer = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
}

void cleanup_cursors(void) {
  if (cursor_pointer) SDL_DestroyCursor(cursor_pointer);
  if (cursor_default) SDL_DestroyCursor(cursor_default);
}

bool cursor_in_rect(float x, float y, SDL_FRect *r) {
  return x >= r->x && x < (r->x + r->w) && y >= r->y && y < (r->y + r->h);
}

