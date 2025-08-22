#include "ui.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text, SDL_Color color, float x, float y) {
  SDL_Surface *surf = TTF_RenderText_Blended(font, text, 0, color);
  SDL_Texture *tex  = SDL_CreateTextureFromSurface(r, surf);
  SDL_FRect dst     = {x, y, (float)surf->w, (float)surf->h};
  SDL_DestroySurface(surf);
  SDL_RenderTexture(r, tex, NULL, &dst);
  SDL_DestroyTexture(tex);
}

void draw_text_centered(SDL_Renderer *r, TTF_Font *font, const char *text, SDL_Color color, SDL_FRect rect) {
  int text_w = 0;
  TTF_MeasureString(font, text, 0, 0, &text_w, NULL);

  float x = rect.x + (rect.w - text_w) / 2.0f;
  float y = rect.y + (rect.h - TTF_GetFontHeight(font)) / 2.0f;

  draw_text(r, font, text, color, x, y);
}

int wrap_text_into_lines(TTF_Font *font, const char *text, float max_w, char lines[][MAX_FEN], int max_lines) {
  if (!font || !text) return 0;

  int len = (int)SDL_strlen(text);
  int start = 0;
  int line_count = 0;

  while (start < len && line_count < max_lines) {
    int end = start;
    int w = 0;
    char tmp[MAX_FEN];

    while (end < len) {
      int chunk = end - start + 1;
      if (chunk >= MAX_FEN) break;
      SDL_memcpy(tmp, &text[start], chunk);
      tmp[chunk] = '\0';
      TTF_MeasureString(font, tmp, 0, 0, &w, NULL);
      if (w > (int)max_w) break;
      end++;
    }

    if (end == start) {
      int take = 1;
      if (start + take > len) take = len - start;
      SDL_memcpy(tmp, &text[start], take);
      tmp[take] = '\0';
      SDL_strlcpy(lines[line_count++], tmp, MAX_FEN);
      start += take;
    } else {
      int take = end - start;
      if (take >= MAX_FEN) take = MAX_FEN - 1;
      SDL_memcpy(tmp, &text[start], take);
      tmp[take] = '\0';
      SDL_strlcpy(lines[line_count++], tmp, MAX_FEN);
      start = end;
    }
  }

  return line_count;
}