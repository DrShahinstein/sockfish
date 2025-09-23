#pragma once

#include "ui.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text, SDL_Color color, float x, float y);
void draw_text_centered(SDL_Renderer *r, TTF_Font *font, const char *text, SDL_Color color, SDL_FRect rect);
int wrap_text_into_lines(TTF_Font *font, const char *text, float max_w, char **lines, int max_lines, int max_buf_size);
void render_text_input(SDL_Renderer *r, UI_TextInput *ui_text_input, FontMenu *fonts);