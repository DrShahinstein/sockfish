#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>

extern SDL_Cursor *cursor_default;
extern SDL_Cursor *cursor_pointer;
extern SDL_Cursor *cursor_text;

void init_cursors(void);
void cleanup_cursors(void);
bool cursor_in_rect(float x, float y, SDL_FRect *r);

extern SDL_Renderer *g_renderer;
void get_mouse_pos(float *x, float *y);