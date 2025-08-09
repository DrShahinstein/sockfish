#include "ui.h"
#include "window.h"
#include "cursor.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string.h>

// font colors
#define FWHITE (SDL_Color){255,255,255,255}
#define FBLACK (SDL_Color){0,0,0,255}

static void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text, SDL_Color color, float x, float y) {
  SDL_Surface *surf = TTF_RenderText_Blended(font, text, strlen(text)+1, color);
  SDL_Texture *tex  = SDL_CreateTextureFromSurface(r, surf);
  SDL_FRect dst     = {x, y, (float)surf->w, (float)surf->h};
  SDL_DestroySurface(surf);
  SDL_RenderTexture(r, tex, NULL, &dst);
  SDL_DestroyTexture(tex);
}

void ui_init(UI_State *ui) {
  ui->font                   = TTF_OpenFont(ROBOTO, 16);
  ui->engine_on              = false;
  ui->toggle_button.rect     = (SDL_FRect){BOARD_SIZE + UI_PADDING, UI_PADDING, 30, 30};
  ui->toggle_button.hovered  = false;
  ui->toggle_button.active   = false;
}

void ui_draw(SDL_Renderer *r, UI_State *ui) {
  // panel
  SDL_FRect panel = {BOARD_SIZE, 0, UI_WIDTH, BOARD_SIZE};
  SDL_SetRenderDrawColor(r, 40, 44, 52, 255);
  SDL_RenderFillRect(r, &panel);

  // toggler
  SDL_FRect tog = ui->toggle_button.rect;
  SDL_SetRenderDrawColor(r, ui->engine_on ? 100 : 200, ui->engine_on ? 200 : 100, 100, 255);
  SDL_RenderFillRect(r, &tog);
  draw_text(r, ui->font, ui->engine_on ? "Engine: ON" : "Engine: OFF", FWHITE, tog.x + tog.w + 10, tog.y + 5);
}

void ui_handle_event(UI_State *ui, SDL_Event *e) {
  float mx, my;
  static bool last_over;

  switch (e->type) {
  case SDL_EVENT_MOUSE_MOTION:
    mx = (float)e->motion.x;
    my = (float)e->motion.y;

    bool over_toggler = cursor_in_rect(mx, my, &ui->toggle_button.rect);
    bool over_board   = (mx >= 0.0f && mx < BOARD_SIZE && my >= 0.0f && my < BOARD_SIZE);
    bool over_any     = over_toggler || over_board;

    if (over_any != last_over) {
      SDL_SetCursor(over_any ? cursor_pointer : cursor_default);
      last_over = over_any;
    }

    ui->toggle_button.hovered = over_toggler;
    break;

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    if (e->button.button == SDL_BUTTON_LEFT) {
      SDL_GetMouseState(&mx, &my);
      if (cursor_in_rect(mx, my, &ui->toggle_button.rect)) {
        ui->toggle_button.active = true;
      }
    }
    break;

  case SDL_EVENT_MOUSE_BUTTON_UP:
    if (e->button.button == SDL_BUTTON_LEFT) {
      SDL_GetMouseState(&mx, &my);
      if (ui->toggle_button.active && cursor_in_rect(mx, my, &ui->toggle_button.rect)) {
        ui->engine_on = !ui->engine_on;
      }
      ui->toggle_button.active = false;
    }
    break;
  }
}

void ui_destroy(UI_State *ui) {
  (void)ui;
}
