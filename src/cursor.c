#include "cursor.h"
#include <SDL3/SDL_mouse.h>

void cleanup_cursors() {
  if (CURSOR_POINTER) SDL_DestroyCursor(CURSOR_POINTER);
  if (CURSOR_DEFAULT) SDL_DestroyCursor(CURSOR_DEFAULT);
}
