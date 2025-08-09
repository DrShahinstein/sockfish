#include "ui.h"
#include "window.h"
#include "cursor.h"
#include "board.h"
#include "game.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string.h>

// font colors
#define FWHITE (SDL_Color){255,255,255,255}
#define FBLACK (SDL_Color){0,0,0,255}
#define FGRAY  (SDL_Color){150,150,150,255}

static void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text, SDL_Color color, float x, float y) {
  SDL_Surface *surf = TTF_RenderText_Blended(font, text, strlen(text)+1, color);
  SDL_Texture *tex  = SDL_CreateTextureFromSurface(r, surf);
  SDL_FRect dst     = {x, y, (float)surf->w, (float)surf->h};
  SDL_DestroySurface(surf);
  SDL_RenderTexture(r, tex, NULL, &dst);
  SDL_DestroyTexture(tex);
}

static void draw_text_centered(SDL_Renderer *r, TTF_Font *font, const char *text, SDL_Color color, SDL_FRect rect) {
  int text_w = 0;
  TTF_MeasureString(font, text, 0, 0, &text_w, NULL);

  float x = rect.x + (rect.w - text_w) / 2.0f;
  float y = rect.y + (rect.h - TTF_GetFontHeight(font)) / 2.0f;

  draw_text(r, font, text, color, x, y);
}

void ui_init(UI_State *ui) {
  ui->font                    = TTF_OpenFont(ROBOTO, 16);
  ui->engine_on               = false;
  ui->toggle_btn.rect         = (SDL_FRect){BOARD_SIZE + UI_PADDING, UI_PADDING, 30, 30};
  ui->toggle_btn.hovered      = false;
  ui->toggle_btn.active       = false;
  ui->fen_loader.area.rect    = (SDL_FRect){BOARD_SIZE + UI_PADDING, UI_PADDING+50, 220, 60};
  ui->fen_loader.area.active  = false;
  ui->fen_loader.area.hovered = false; 
  ui->fen_loader.length       = 0;
  ui->fen_loader.input[0]     = '\0';
  ui->fen_loader.btn.rect     = (SDL_FRect){ui->fen_loader.area.rect.x, ui->fen_loader.area.rect.y + 70, 110, 30};
  ui->fen_loader.btn.active   = false;
  ui->fen_loader.btn.hovered  = false;

  if (!ui->font) SDL_Log("Could not load font: %s", SDL_GetError());
}

void ui_draw(SDL_Renderer *r, UI_State *ui) {
  // panel
  SDL_FRect panel = {BOARD_SIZE, 0, UI_WIDTH, BOARD_SIZE};
  SDL_SetRenderDrawColor(r, 40, 44, 52, 255);
  SDL_RenderFillRect(r, &panel);

  // toggler
  SDL_FRect tog = ui->toggle_btn.rect;
  SDL_SetRenderDrawColor(r, ui->engine_on ? 100 : 200, ui->engine_on ? 200 : 100, 100, 255);
  SDL_RenderFillRect(r, &tog);
  draw_text(r, ui->font, ui->engine_on ? "Engine: ON" : "Engine: OFF", FWHITE, tog.x + tog.w + 10, tog.y + 5);

  // --- fen loader ---
  SDL_FRect fen = ui->fen_loader.area.rect;
  SDL_SetRenderDrawColor(r, 255, 255, 255, 0);
  SDL_RenderFillRect(r, &fen);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &fen);

  float text_x = fen.x + 6;
  float text_y = fen.y + 6;

  if (ui->fen_loader.length == 0 && !ui->fen_loader.area.active) draw_text(r, ui->font, FEN_PLACEHOLDER, FGRAY, text_x, text_y);
  else if (ui->fen_loader.length > 0)                            draw_text(r, ui->font, ui->fen_loader.input, FBLACK, text_x, text_y);
  else {}
  if (ui->fen_loader.area.active) {
    int w = 0;
    int h = TTF_GetFontHeight(ui->font);
    
    if (ui->fen_loader.length > 0) TTF_MeasureString(ui->font, ui->fen_loader.input, 0, 0, &w, NULL);

    bool caret_visible = SDL_GetTicks() % 1000 < 500;

    if (caret_visible) {
      SDL_FRect caret = {text_x + (float)w, text_y, 2.0f, (float)h};
      SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
      SDL_RenderFillRect(r, &caret);
    }
  }

  SDL_FRect fenbtn = ui->fen_loader.btn.rect;
  SDL_SetRenderDrawColor(r, ui->fen_loader.btn.hovered ? 170 : 200, 200, 200, 255);
  SDL_RenderFillRect(r, &fenbtn);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &fenbtn);
  draw_text_centered(r, ui->font, "Load", FBLACK, ui->fen_loader.btn.rect);
}

void ui_handle_event(SDL_Event *e, UI_State *ui, GameState *game) {
  float mx, my;
  static bool last_over;
  static bool last_over_fen_area;

  switch (e->type) {
  case SDL_EVENT_MOUSE_MOTION:
    mx = (float)e->motion.x;
    my = (float)e->motion.y;
   
    bool over_board    = (mx >= 0.0f && mx < BOARD_SIZE && my >= 0.0f && my < BOARD_SIZE);
    bool over_toggler  = cursor_in_rect(mx, my, &ui->toggle_btn.rect);
    bool over_fen_area = cursor_in_rect(mx, my, &ui->fen_loader.area.rect);
    bool over_fen_btn  = cursor_in_rect(mx, my, &ui->fen_loader.btn.rect);
    bool over_any      = over_board || over_toggler || over_fen_btn;

    if (over_any != last_over) {
      SDL_SetCursor(over_any ? cursor_pointer : cursor_default);
      last_over = over_any;
    }

    if (over_fen_area != last_over_fen_area) {
      SDL_SetCursor(over_fen_area ? cursor_text : cursor_default);
      last_over_fen_area = over_fen_area;
    }

    ui->toggle_btn.hovered      = over_toggler;
    ui->fen_loader.area.hovered = over_fen_area;
    ui->fen_loader.btn.hovered  = over_fen_btn;
    break;

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    if (e->button.button == SDL_BUTTON_LEFT) {
      SDL_GetMouseState(&mx, &my);

      if (cursor_in_rect(mx, my, &ui->toggle_btn.rect)) ui->toggle_btn.active = true;
      else if (cursor_in_rect(mx, my, &ui->fen_loader.area.rect)) {
        ui->fen_loader.area.active = true;
        SDL_StartTextInput(SDL_GetKeyboardFocus());
      }
      else if (ui->fen_loader.area.active) {
        ui->fen_loader.area.active = false;
        SDL_StopTextInput(SDL_GetKeyboardFocus());
      }
      else if (cursor_in_rect(mx, my, &ui->fen_loader.btn.rect)) ui->fen_loader.btn.active = true;
    }
    break;

  case SDL_EVENT_MOUSE_BUTTON_UP:
    if (e->button.button == SDL_BUTTON_LEFT) {
      SDL_GetMouseState(&mx, &my);

      if (ui->toggle_btn.active && cursor_in_rect(mx, my, &ui->toggle_btn.rect)) ui->engine_on = !ui->engine_on;
      ui->toggle_btn.active = false;

      if (ui->fen_loader.btn.active && cursor_in_rect(mx, my, &ui->fen_loader.btn.rect)) {
        if (ui->fen_loader.length > 0) load_fen(ui->fen_loader.input, game);
      }

      ui->fen_loader.btn.active = false;
    }
    break;

  case SDL_EVENT_TEXT_INPUT:
    if (ui->fen_loader.area.active) {
      const char *txt = e->text.text;
      size_t avail = MAX_FEN - ui->fen_loader.length - 1;
      if (avail > 0) {
        strncat(ui->fen_loader.input, txt, avail);
        ui->fen_loader.length = strlen(ui->fen_loader.input);
      }
    }
    break;

  case SDL_EVENT_KEY_DOWN:
    if (ui->fen_loader.area.active) {
      SDL_Keycode kc = e->key.key;

      if (kc == SDLK_BACKSPACE) {
        if (ui->fen_loader.length > 0) {
          ui->fen_loader.length--;
          ui->fen_loader.input[ui->fen_loader.length] = '\0';
        }
      } else if (kc == SDLK_RETURN || kc == SDLK_KP_ENTER) {
        if (ui->fen_loader.length > 0 && game) {
          load_fen(ui->fen_loader.input, game);
        }
      } else if (kc == SDLK_V && (SDL_GetModState() & SDL_KMOD_CTRL)) {
        char *clip = SDL_GetClipboardText();

        if (clip) {
          size_t avail = MAX_FEN - ui->fen_loader.length - 1;
          if (avail > 0) {
            strncat(ui->fen_loader.input, clip, avail);
            ui->fen_loader.length = strlen(ui->fen_loader.input);
          }
          SDL_free(clip);
        }
      }
    }
    break;
  }
}

void ui_destroy(UI_State *ui) {
  if (ui->font) {
    TTF_CloseFont(ui->font);
    ui->font = NULL;
  }

  SDL_StopTextInput(SDL_GetKeyboardFocus());
}
