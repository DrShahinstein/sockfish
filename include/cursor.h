#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>

#define CURSOR_DEFAULT SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT)
#define CURSOR_POINTER SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER)

void cleanup_cursors() {
  if (CURSOR_POINTER) SDL_DestroyCursor(CURSOR_POINTER);
  if (CURSOR_DEFAULT) SDL_DestroyCursor(CURSOR_DEFAULT);
}
