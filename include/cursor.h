#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>

#ifndef CURSOR_DEFAULT
#define CURSOR_DEFAULT SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT)
#endif

#ifndef CURSOR_POINTER
#define CURSOR_POINTER SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER)
#endif

void cleanup_cursors();
