#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>

extern SDL_Cursor *cursor_default;
extern SDL_Cursor *cursor_pointer;

void init_cursors(void);
void cleanup_cursors(void);
bool cursor_in_rect(float x, float y, SDL_FRect *r);
