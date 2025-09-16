#include "ui.h"
#include "cursor.h"
#include <SDL3/SDL.h>

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
    bool over_undo_btn  = cursor_in_rect(mx, my, &ui->undo_btn.rect);
    bool over_any       = over_board || over_toggler || over_fen_btn || over_reset_btn || over_tchanger || over_undo_btn;

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
    ui->undo_btn.hovered        = over_undo_btn;
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

      if (cursor_in_rect(mx, my, &ui->engine_toggler.rect)) {
        ui->engine_on = !ui->engine_on;
      }

      if (cursor_in_rect(mx, my, &ui->fen_loader.btn.rect)) {
        if (ui->fen_loader.length > 0) {
          load_board(ui->fen_loader.input, board);
        }
      }

      if (cursor_in_rect(mx, my, &ui->reset_btn.rect)) {
        load_board(START_FEN, board);
      }

      if (cursor_in_rect(mx, my, &ui->undo_btn.rect)) {
        board_undo(board);
      }

      if (cursor_in_rect(mx, my, &ui->turn_changer.rect)) {
        if  (board->turn == WHITE) board->turn = BLACK;
        else board->turn = WHITE;
      } 
    }
    break;

  case SDL_EVENT_TEXT_INPUT:
    if (ui->fen_loader.active) {
      const char *txt = e->text.text;
      size_t avail = MAX_FEN - ui->fen_loader.length - 1;
      if (avail > 0) {
        SDL_strlcat(ui->fen_loader.input, txt, avail);
        ui->fen_loader.length = SDL_strlen(ui->fen_loader.input);
      }
    }
    break;

  case SDL_EVENT_KEY_DOWN:
    if (e->key.key == SDLK_LEFT) {
      board_undo(board);
    }

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
            SDL_strlcat(ui->fen_loader.input, clip, avail);
            ui->fen_loader.length = SDL_strlen(ui->fen_loader.input);
          }
          SDL_free(clip);
        }
      }
    }
    break;
  }
}