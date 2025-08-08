#include "cursor.h"

SDL_Cursor *cursor_default = NULL;
SDL_Cursor *cursor_pointer = NULL;

void init_cursors(void) {
  cursor_default = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
  cursor_pointer = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
  SDL_SetCursor(cursor_default);
}

void cleanup_cursors(void) {
  if (cursor_pointer) SDL_DestroyCursor(cursor_pointer);
  if (cursor_default) SDL_DestroyCursor(cursor_default);
}
