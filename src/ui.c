#include "ui.h"
#include "window.h"
#include "cursor.h"
#include "board.h"
#include "ui_helpers.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string.h>

// font colors
#define FWHITE (SDL_Color){255,255,255,255}
#define FBLACK (SDL_Color){0,0,0,255}
#define FGRAY  (SDL_Color){150,150,150,255}

void ui_init(UI_State *ui) {
  ui->font                    = TTF_OpenFont(ROBOTO,16);
  ui->engine_on               = false;
  ui->engine_toggler.rect     = (SDL_FRect){BOARD_SIZE+UI_PADDING,UI_PADDING,30,30};
  ui->engine_toggler.hovered  = false;
  ui->fen_loader.font         = TTF_OpenFont(JBMONO,14);
  ui->fen_loader.area.rect    = (SDL_FRect){BOARD_SIZE+UI_PADDING,UI_PADDING+50,230,90};
  ui->fen_loader.active       = false;
  ui->fen_loader.area.hovered = false; 
  ui->fen_loader.length       = 0;
  ui->fen_loader.input[0]     = '\0';
  ui->fen_loader.btn.rect     = (SDL_FRect){ui->fen_loader.area.rect.x,ui->fen_loader.area.rect.y+100,100,30};
  ui->fen_loader.btn.hovered  = false;
  ui->reset_btn.rect          = (SDL_FRect){ui->fen_loader.area.rect.x+ui->fen_loader.area.rect.w-100,ui->fen_loader.btn.rect.y,100,ui->fen_loader.btn.rect.h};
  ui->reset_btn.hovered       = false;
  ui->separator.rect          = (SDL_FRect){ui->fen_loader.area.rect.x,ui->reset_btn.rect.y+80,ui->fen_loader.area.rect.w,4};
  ui->turn                    = WHITE;
  ui->turn_changer.rect       = (SDL_FRect){ui->fen_loader.area.rect.x,ui->separator.rect.y+25,30,30};
  ui->turn_changer.hovered    = false;

  if (!ui->font)            SDL_Log("Could not load font: %s", SDL_GetError());
  if (!ui->fen_loader.font) SDL_Log("Could not load font: %s", SDL_GetError());
}

void ui_draw(SDL_Renderer *r, UI_State *ui, Sockfish *sockfish, BoardState *board) {
  // panel
  SDL_FRect panel = {BOARD_SIZE, 0, UI_WIDTH, BOARD_SIZE};
  SDL_SetRenderDrawColor(r, 40, 44, 52, 255);
  SDL_RenderFillRect(r, &panel);

  // toggler
  SDL_FRect tog = ui->engine_toggler.rect;
  SDL_SetRenderDrawColor(r, ui->engine_on ? 100 : 200, ui->engine_on ? 200 : 100, 100, 255);
  SDL_RenderFillRect(r, &tog);
  draw_text(r, ui->font, ui->engine_on ? "Engine: ON" : "Engine: OFF", FWHITE, tog.x + tog.w + 10, tog.y + 5);

  /* --- fen loader --- */
  SDL_FRect fen = ui->fen_loader.area.rect;
  SDL_SetRenderDrawColor(r, 255, 255, 255, 0);
  SDL_RenderFillRect(r, &fen);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &fen);

  float text_x = fen.x + 6;
  float text_y = fen.y + 6;

  int line_h = TTF_GetFontHeight(ui->fen_loader.font);
  if (line_h <= 0) line_h = 16;

  int max_vis_lines = (int) (fen.h / (float)line_h);
  if (max_vis_lines <= 0) max_vis_lines = 1;

  const int MAX_WRAP_LINES = 10;
  char wrapped[MAX_WRAP_LINES][MAX_FEN];
  for (int i = 0; i < MAX_WRAP_LINES; ++i) wrapped[i][0] = '\0';

  int total_lines = wrap_text_into_lines(ui->fen_loader.font, ui->fen_loader.input, fen.w - 8.0f, wrapped, MAX_WRAP_LINES);
  int first_line = 0;
  if (total_lines > max_vis_lines) {
    if (ui->fen_loader.active) first_line = total_lines - max_vis_lines;
    else first_line = 0;
  }

  int last_line = first_line + max_vis_lines;
  if (last_line > total_lines) last_line = total_lines;

  for (int li = first_line, row_i = 0; li < last_line; ++li, ++row_i) {
    float y = text_y + row_i * (float)line_h;
    draw_text(r, ui->fen_loader.font, wrapped[li], FBLACK, text_x, y);
  }

  if (ui->fen_loader.length == 0 && !ui->fen_loader.active) draw_text(r, ui->font, FEN_PLACEHOLDER, FGRAY, text_x, text_y);
  else if (ui->fen_loader.length > 0)                       draw_text(r, ui->fen_loader.font, ui->fen_loader.input, FBLACK, text_x, text_y);
  else {}
  if (ui->fen_loader.active) {
    int w; int visible_count;
    bool caret_visible = SDL_GetTicks() % 1000 < 500;

    if (caret_visible) {
      visible_count = last_line - first_line;
      int last_visible_idx = (visible_count > 0) ? (last_line - 1) : -1; 
      w = 0;
      if (last_visible_idx >= 0) TTF_MeasureString(ui->fen_loader.font, wrapped[last_visible_idx], 0, 0, &w, NULL);
      else w = 0;
    }

    float caret_x = text_x + (float)w;
    float caret_y = text_y + (float) ((visible_count > 0 ? visible_count-1 : 0) * line_h);
    SDL_FRect caret = { caret_x, caret_y, 2.0f, (float)line_h };
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderFillRect(r, &caret);
  }

  SDL_FRect fenbtn = ui->fen_loader.btn.rect;
  SDL_SetRenderDrawColor(r, ui->fen_loader.btn.hovered ? 170 : 200, 200, 200, 255);
  SDL_RenderFillRect(r, &fenbtn);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &fenbtn);
  draw_text_centered(r, ui->font, "Load", FBLACK, ui->fen_loader.btn.rect);

  SDL_FRect resetbtn = ui->reset_btn.rect; 
  SDL_SetRenderDrawColor(r, ui->reset_btn.hovered ? 170 : 200, 200, 200, 255);
  SDL_RenderFillRect(r, &resetbtn);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &resetbtn);
  draw_text_centered(r, ui->font, "Reset", FBLACK, ui->reset_btn.rect);

  /* --- sockfish engine --- */
  if (ui->engine_on) {
    SDL_FRect separator = ui->separator.rect;
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderFillRect(r, &separator);
    SDL_RenderRect(r, &separator);

    SDL_FRect turn_changer = ui->turn_changer.rect;
    bool white = ui->turn == WHITE;
    if (white) SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    else SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
    SDL_RenderFillRect(r, &turn_changer);
    draw_text(r, ui->font, white ? "White to play" : "Black to play", FWHITE, turn_changer.x + turn_changer.w + 10, turn_changer.y + 5);

    sf_req_search(sockfish, board, ui->turn);

    SDL_LockMutex(sockfish->mtx);
    Move best = sockfish->best;
    bool thinking = sockfish->thinking;
    SDL_UnlockMutex(sockfish->mtx);

    float move_x = ui->turn_changer.rect.x;
    float move_y = ui->turn_changer.rect.y + 50;

    if (thinking) {
      draw_text(r, ui->font, "Thinking...", FWHITE, move_x, move_y);
    }
    else if (best.fr >= 0 && best.fc >= 0 && best.tr >= 0 && best.tc >= 0)
    {
      char from_file = (char)('e' + (best.fc));
      char from_rank = (char)('2' - (best.fr));
      char to_file = (char)('e' + (best.tc));
      char to_rank = (char)('4' - (best.tr));
      char move_str[32];

      SDL_snprintf(move_str, sizeof move_str, "Best: %c%c -> %c%c", from_file, from_rank, to_file, to_rank);
      draw_text(r, ui->font, move_str, FWHITE, move_x, move_y);
    }
  }
}

void ui_handle_event(SDL_Event *e, UI_State *ui, BoardState *board) {
  float mx, my;
  static bool last_over;
  static bool last_over_fen_area;

  switch (e->type) {
  case SDL_EVENT_MOUSE_MOTION:
    mx = (float)e->motion.x;
    my = (float)e->motion.y;
   
    bool over_board     = (mx >= 0.0f && mx < BOARD_SIZE && my >= 0.0f && my < BOARD_SIZE);
    bool over_toggler   = cursor_in_rect(mx, my, &ui->engine_toggler.rect);
    bool over_tchanger  = cursor_in_rect(mx, my, &ui->turn_changer.rect) && ui->engine_on;
    bool over_fen_area  = cursor_in_rect(mx, my, &ui->fen_loader.area.rect);
    bool over_fen_btn   = cursor_in_rect(mx, my, &ui->fen_loader.btn.rect);
    bool over_reset_btn = cursor_in_rect(mx, my, &ui->reset_btn.rect);
    bool over_any       = over_board || over_toggler || over_fen_btn || over_reset_btn || over_tchanger;

    if (over_any != last_over) {
      SDL_SetCursor(over_any ? cursor_pointer : cursor_default);
      last_over = over_any;
    }

    if (over_fen_area != last_over_fen_area) {
      SDL_SetCursor(over_fen_area ? cursor_text : cursor_default);
      last_over_fen_area = over_fen_area;
    }

    ui->engine_toggler.hovered  = over_toggler;
    ui->turn_changer.hovered    = over_tchanger;
    ui->fen_loader.area.hovered = over_fen_area;
    ui->fen_loader.btn.hovered  = over_fen_btn;
    ui->reset_btn.hovered       = over_reset_btn;
    break;

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    if (e->button.button == SDL_BUTTON_LEFT) {
      SDL_GetMouseState(&mx, &my);

      if (cursor_in_rect(mx, my, &ui->fen_loader.area.rect)) {
        ui->fen_loader.active = true;
        SDL_StartTextInput(SDL_GetKeyboardFocus());
      }
      else if (ui->fen_loader.active) {
        ui->fen_loader.active = false;
        SDL_StopTextInput(SDL_GetKeyboardFocus());
      }
    }
    break;

  case SDL_EVENT_MOUSE_BUTTON_UP:
    if (e->button.button == SDL_BUTTON_LEFT) {
      SDL_GetMouseState(&mx, &my);

      if (cursor_in_rect(mx, my, &ui->engine_toggler.rect)) ui->engine_on = !ui->engine_on;

      if (cursor_in_rect(mx, my, &ui->fen_loader.btn.rect)) {
        if (ui->fen_loader.length > 0) load_board(ui->fen_loader.input, board);
      }

      if (cursor_in_rect(mx, my, &ui->reset_btn.rect)) load_board(START_FEN, board);

      if (cursor_in_rect(mx, my, &ui->turn_changer.rect)) {
        if (ui->turn == WHITE) ui->turn = BLACK;
        else ui->turn = WHITE;
      } 
    }
    break;

  case SDL_EVENT_TEXT_INPUT:
    if (ui->fen_loader.active) {
      const char *txt = e->text.text;
      size_t avail = MAX_FEN - ui->fen_loader.length - 1;
      if (avail > 0) {
        strncat(ui->fen_loader.input, txt, avail);
        ui->fen_loader.length = strlen(ui->fen_loader.input);
      }
    }
    break;

  case SDL_EVENT_KEY_DOWN:
    if (ui->fen_loader.active) {
      SDL_Keycode kc = e->key.key;

      if (kc == SDLK_BACKSPACE) {
        if (ui->fen_loader.length > 0) {
          ui->fen_loader.length--;
          ui->fen_loader.input[ui->fen_loader.length] = '\0';
        }
      }

      else if (kc == SDLK_RETURN || kc == SDLK_KP_ENTER) {
        if (ui->fen_loader.length > 0) load_board(ui->fen_loader.input, board);
      }

      else if (kc == SDLK_A && (SDL_GetModState() & SDL_KMOD_CTRL)) {
        ui->fen_loader.input[0] = '\0';
        ui->fen_loader.length = 0;
      }
      
      else if (kc == SDLK_V && (SDL_GetModState() & SDL_KMOD_CTRL)) {
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
