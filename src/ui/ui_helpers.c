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

int wrap_text_into_lines(TTF_Font *font, const char *text, float max_w, char **lines, int max_lines, int max_buf_size) {
  if (!font || !text || !lines || max_buf_size <= 0)
    return 0;

  int len        = (int)SDL_strlen(text);
  int start      = 0;
  int line_count = 0;

  while (start < len && line_count < max_lines) {
    int end   = start;
    int w     = 0;
    char *tmp = (char *)SDL_malloc(max_buf_size);

    if (!tmp)
      break;

    while (end < len) {
      int chunk = end - start + 1;
      if (chunk >= max_buf_size) break;

      SDL_strlcpy(tmp, &text[start], chunk + 1);
      tmp[chunk] = '\0';

      TTF_MeasureString(font, tmp, 0, 0, &w, NULL);
      if (w > (int)max_w)
        break;

      end++;
    }

    if (end == start) {
      int take = 1;
      if (start + take > len) take = len - start;
      SDL_strlcpy(lines[line_count], &text[start], SDL_min(take + 1, max_buf_size));
      line_count++;
      start += take;
    } else {
      int take = end - start;
      if (take >= max_buf_size) take = max_buf_size - 1;
      SDL_strlcpy(lines[line_count], &text[start], take + 1);
      line_count++;
      start = end;
    }

    SDL_free(tmp);
  }

  return line_count;
}

void render_text_input(SDL_Renderer *r, UI_TextInput *ui_text_input, FontMenu *fonts) {
  SDL_FRect rect = ui_text_input->area.rect;
  SDL_SetRenderDrawColor(r, 255, 255, 255, 0);
  SDL_RenderFillRect(r, &rect);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &rect);

  float text_x = rect.x + 6;
  float text_y = rect.y + 6;

  int line_h = TTF_GetFontHeight(fonts->jbmono14);
  if (line_h <= 0) line_h = 16;

  int max_vis_lines = (int) (rect.h / (float)line_h);
  if (max_vis_lines <= 0) max_vis_lines = 1;

  const int MAX_WRAP_LINES = (ui_text_input->type == FEN) ? 3 : 10;
  int max_buf_size         = (ui_text_input->type == FEN) ? MAX_FEN : MAX_PGN;
  char **wrapped           = (char **)SDL_malloc(MAX_WRAP_LINES * sizeof(char *));

  for (int i = 0; i < MAX_WRAP_LINES; ++i) {
    wrapped[i]    = (char *)SDL_malloc(max_buf_size);
    wrapped[i][0] = '\0';
  }

  int total_lines
    = wrap_text_into_lines(fonts->jbmono14, ui_text_input->buf, rect.w - 8.0f, wrapped, MAX_WRAP_LINES, max_buf_size);

  int first_line = 0;
  if (total_lines > max_vis_lines) {
    if (ui_text_input->active) first_line = total_lines - max_vis_lines;
    else first_line = 0;
  }

  int last_line = first_line + max_vis_lines;
  if (last_line > total_lines) last_line = total_lines;

  for (int li = first_line, row_i = 0; li < last_line; ++li, ++row_i) {
    float y = text_y + row_i * (float)line_h;
    draw_text(r, fonts->jbmono14, wrapped[li], FBLACK, text_x, y);
  }

  if (ui_text_input->length == 0 && !ui_text_input->active) {
    draw_text(r, fonts->roboto15, ui_text_input->placeholder, FGRAY, text_x, text_y);
  }

  else if (ui_text_input->length > 0) {
    draw_text(r, fonts->jbmono14, ui_text_input->buf, FBLACK, text_x, text_y);
  }

  else {}

  if (ui_text_input->active) {
    int w; int visible_count;
    bool caret_visible = SDL_GetTicks() % 1000 < 500;

    if (caret_visible) {
      visible_count = last_line - first_line;
      int last_visible_idx = (visible_count > 0) ? (last_line - 1) : -1; 
      w = 0;
      if (last_visible_idx >= 0) TTF_MeasureString(fonts->jbmono14, wrapped[last_visible_idx], 0, 0, &w, NULL);
      else w = 0;
    }

    float caret_x = text_x + (float)w;
    float caret_y = text_y + (float) ((visible_count > 0 ? visible_count-1 : 0) * line_h);
    SDL_FRect caret = { caret_x, caret_y, 2.0f, (float)line_h };
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderFillRect(r, &caret);
  }

  for (int i = 0; i < MAX_WRAP_LINES; ++i) SDL_free(wrapped[i]);
  SDL_free(wrapped);
}