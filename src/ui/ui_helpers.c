#include "ui.h"
#include "ui_render.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

static size_t hash_string(const char *str);

void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text, SDL_Color color, float x, float y) {
  float width, height;
  SDL_Texture *tex = get_text_cache(font, text, color, &width, &height);

  if (tex) {
    SDL_FRect dst = {x, y, width, height};
    SDL_RenderTexture(r, tex, NULL, &dst);
  }
}

void draw_text_centered(SDL_Renderer *r, TTF_Font *font, const char *text, SDL_Color color, SDL_FRect rect) {
  float text_w = 0, text_h = 0;

  get_text_cache(font, text, color, &text_w, &text_h);

  float x = rect.x + (rect.w - text_w) / 2.0f;
  float y = rect.y + (rect.h - text_h) / 2.0f;

  draw_text(r, font, text, color, x, y);
}

int wrap_text_into_lines(TTF_Font *font, const char *text, float max_w, char **lines, int max_lines, int max_buf_size) {
  if (!font || !text || !lines || max_buf_size <= 0)
    return 0;

  int len        = (int)SDL_strlen(text);
  int start      = 0;
  int line_count = 0;

  while (start < len && line_count < max_lines) {
    int end        = start;
    int last_space = -1;
    int w          = 0;

    while (end < len) {
      if (text[end] == ' ') {
        last_space = end;
      }

      int chunk_len = end - start + 1;
      if (chunk_len >= max_buf_size) break;

      char temp_char = text[end + 1];
      ((char *)text)[end + 1] = '\0';

      TTF_MeasureString(font, &text[start], 0, 0, &w, NULL);

      ((char *)text)[end + 1] = temp_char;

      if (w > (int)max_w) {
        if (last_space > start && last_space < end) {
          end = last_space;
        }
        break;
      }

      end++;
    }

    if (end > start) {
      int copy_len = SDL_min(end - start, max_buf_size - 1);
      SDL_strlcpy(lines[line_count], &text[start], copy_len + 1);
      line_count++;

      if (start < len && text[end] == ' ') end++;
      start = end;
    }
    
    else {
      if (start < len) {
        lines[line_count][0] = text[start];
        lines[line_count][1] = '\0';
        line_count++;
        start++;
      } else {
        break;
      }
    }
  }

  return line_count;
}

void render_text_input(SDL_Renderer *r, UI_TextInput *ui_text_input, FontMenu *fonts) {
  SDL_FRect rect = ui_text_input->area.rect;

  SDL_SetRenderDrawColor(r, 255, 255, 255, 0);
  SDL_RenderFillRect(r, &rect);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &rect);

  float text_x     = rect.x + 6;
  float text_y     = rect.y + 6;
  float wrap_width = rect.w - 8.0f;

  int line_h = TTF_GetFontHeight(fonts->jbmono14);
  if (line_h <= 0) line_h = 16;

  int max_vis_lines = (int)(rect.h / (float)line_h);
  if (max_vis_lines <= 0) max_vis_lines = 1;

  // check if we need to recalculate wrapping (saves cpu overhead)
  size_t current_hash = hash_string(ui_text_input->buf);
  bool needs_rewrap   = !ui_text_input->cache_valid                      ||
                        ui_text_input->cached_text_hash  != current_hash ||
                        ui_text_input->cached_wrap_width != wrap_width;

  int total_lines;

  if (needs_rewrap) {
    for (int i = 0; i < ui_text_input->wrap_line_count; ++i) {
      ui_text_input->wrap_lines[i][0] = '\0';
    }

    int max_buf_size = (ui_text_input->type == FEN) ? MAX_FEN : MAX_PGN;

    total_lines =
        wrap_text_into_lines(fonts->jbmono14, ui_text_input->buf, wrap_width,
                             ui_text_input->wrap_lines,
                             ui_text_input->wrap_line_count, max_buf_size);

    ui_text_input->cached_line_count = total_lines;
    ui_text_input->cached_text_hash  = current_hash;
    ui_text_input->cached_wrap_width = wrap_width;
    ui_text_input->cache_valid       = true;
  } else {
    total_lines = ui_text_input->cached_line_count;
  }

  int first_line = 0;
  if (total_lines > max_vis_lines) {
    if (ui_text_input->active) {
      first_line = total_lines - max_vis_lines;
    }
  }

  int last_line = SDL_min(first_line + max_vis_lines, total_lines);

  if (ui_text_input->length == 0 && !ui_text_input->active) {
    draw_text(r, fonts->roboto15, ui_text_input->placeholder, FGRAY, text_x, text_y);
  }
  
  else {
    for (int li = first_line, row_i = 0; li < last_line; ++li, ++row_i) {
      float y = text_y + row_i * (float)line_h;
      draw_text(r, fonts->jbmono14, ui_text_input->wrap_lines[li], FBLACK, text_x, y);
    }
  }

  if (ui_text_input->active && (SDL_GetTicks() % 1000 < 500)) {
    int visible_count = last_line - first_line;
    int last_visible_idx = (visible_count > 0) ? (last_line - 1) : -1;

    int w = 0;
    if (last_visible_idx >= 0 && ui_text_input->wrap_lines[last_visible_idx][0]) {
      TTF_MeasureString(fonts->jbmono14, ui_text_input->wrap_lines[last_visible_idx], 0, 0, &w, NULL);
    }

    float caret_x   = text_x + (float)w;
    float caret_y   = text_y + (float)((visible_count > 0 ? visible_count - 1 : 0) * line_h);
    SDL_FRect caret = {caret_x, caret_y, 2.0f, (float)line_h};
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderFillRect(r, &caret);
  }
}

static size_t hash_string(const char *str) {
  size_t hash = 5381;
  int c;

  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  
  return hash;
}