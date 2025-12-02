#include "ui.h"
#include "cursor.h"
#include <SDL3/SDL.h>

static void handle_text_input_activation(UI_TextInput *ui_text_input, float mx, float my);
static void handle_text_input_keys(SDL_Event *e, UI_TextInput *input, size_t max_len, void (*load_callback)(const char *, BoardState *), BoardState *board);

void ui_handle_event(SDL_Event *e, UI_State *ui, BoardState *board) {
  float mx, my;
  static bool last_over;
  static bool last_over_text_area;

  switch (e->type) {
  case SDL_EVENT_MOUSE_MOTION:
    mx = (float)e->motion.x;
    my = (float)e->motion.y;

    bool over_board     = (mx >= 0.0f && mx < BOARD_SIZE && my >= 0.0f && my < BOARD_SIZE);
    bool over_toggler   = cursor_in_rect(mx, my, &ui->engine_toggler.rect);
    bool over_fen_btn   = cursor_in_rect(mx, my, &ui->fen_loader.btn.rect);
    bool over_pgn_btn   = cursor_in_rect(mx, my, &ui->pgn_loader.btn.rect);
    bool over_tchanger  = cursor_in_rect(mx, my, &ui->turn_changer.rect);
    bool over_achanger  = cursor_in_rect(mx, my, &ui->arrow_changer.rect);
    bool over_reset_btn = cursor_in_rect(mx, my, &ui->reset_btn.rect);
    bool over_undo_btn  = cursor_in_rect(mx, my, &ui->undo_btn.rect);
    bool over_redo_btn  = cursor_in_rect(mx, my, &ui->redo_btn.rect);
    bool over_any       = over_board     || over_toggler  || over_fen_btn  || over_pgn_btn   ||
                          over_reset_btn || over_tchanger || over_undo_btn || over_redo_btn  || over_achanger;

    bool over_fen_area      = cursor_in_rect(mx, my, &ui->fen_loader.area.rect);
    bool over_pgn_area      = cursor_in_rect(mx, my, &ui->pgn_loader.area.rect);
    bool over_any_text_area = over_fen_area || over_pgn_area;

    if (over_any != last_over) {
      SDL_SetCursor(over_any ? cursor_pointer : cursor_default);
      last_over = over_any;
    }

    if (over_any_text_area != last_over_text_area) {
      SDL_SetCursor(over_any_text_area ? cursor_text : cursor_default);
      last_over_text_area = over_any_text_area;
    }

    ui->engine_toggler.hovered  = over_toggler;
    ui->turn_changer.hovered    = over_tchanger;
    ui->arrow_changer.hovered   = over_achanger;
    ui->fen_loader.area.hovered = over_fen_area;
    ui->fen_loader.btn.hovered  = over_fen_btn;
    ui->pgn_loader.area.hovered = over_pgn_area;
    ui->pgn_loader.btn.hovered  = over_pgn_btn;
    ui->reset_btn.hovered       = over_reset_btn;
    ui->undo_btn.hovered        = over_undo_btn;
    ui->redo_btn.hovered        = over_redo_btn;
    break;

  case SDL_EVENT_MOUSE_BUTTON_DOWN:
    if (e->button.button == SDL_BUTTON_LEFT) {
      get_mouse_pos(&mx, &my);

      handle_text_input_activation(&ui->fen_loader, mx, my);
      handle_text_input_activation(&ui->pgn_loader, mx, my);
    }
    break;

  case SDL_EVENT_MOUSE_BUTTON_UP:
    if (e->button.button == SDL_BUTTON_LEFT) {
      get_mouse_pos(&mx, &my);

      if (cursor_in_rect(mx, my, &ui->engine_toggler.rect)) {
        ui->engine_on = !ui->engine_on;
      }

      if (cursor_in_rect(mx, my, &ui->arrow_changer.rect)) {
        ui->arrow_changer.color_idx = (ui->arrow_changer.color_idx + 1) % ARROW_COLORS_COUNT;
        board->annotations.arrow_color = ui->arrow_changer.colors[ui->arrow_changer.color_idx];
      }

      if (cursor_in_rect(mx, my, &ui->fen_loader.btn.rect)) {
        if (ui->fen_loader.length > 0) {
          load_fen(ui->fen_loader.buf, board);
          board_update_king_in_check(board);
        }
      }

      if (cursor_in_rect(mx, my, &ui->pgn_loader.btn.rect)) {
        if (ui->pgn_loader.length > 0) {
          load_pgn(ui->pgn_loader.buf, board);
        }
      }

      if (cursor_in_rect(mx, my, &ui->reset_btn.rect)) {
        load_fen(START_FEN, board);
      }

      if (cursor_in_rect(mx, my, &ui->undo_btn.rect)) {
        board_undo(board);
      }

      if (cursor_in_rect(mx, my, &ui->redo_btn.rect)) {
        board_redo(board);
      }

      if (cursor_in_rect(mx, my, &ui->turn_changer.rect)) {
        board->turn = !board->turn;
      } 
    }
    break;

  case SDL_EVENT_TEXT_INPUT:
    if (ui->fen_loader.active) {
      const char *txt = e->text.text;
      size_t avail = MAX_FEN - ui->fen_loader.length - 1;

      if (avail > 0) {
        SDL_strlcat(ui->fen_loader.buf, txt, avail);
        ui->fen_loader.length      = SDL_strlen(ui->fen_loader.buf);
        ui->fen_loader.cache_valid = false;
      }
    }

    else if (ui->pgn_loader.active) {
      const char *txt = e->text.text;
      size_t avail = MAX_PGN - ui->pgn_loader.length - 1;

      if (avail > 0) {
        SDL_strlcat(ui->pgn_loader.buf, txt, avail);
        ui->pgn_loader.length      = SDL_strlen(ui->pgn_loader.buf);
        ui->pgn_loader.cache_valid = false;
      }
    }
    break;

  case SDL_EVENT_KEY_DOWN:
    handle_text_input_keys(e, &ui->fen_loader, MAX_FEN, load_fen, board);
    handle_text_input_keys(e, &ui->pgn_loader, MAX_PGN, load_pgn, board);

    if (ui->fen_loader.active || ui->pgn_loader.active) return;

    if (e->key.key == SDLK_LEFT) {
      board_undo(board);
    }

    else if (e->key.key == SDLK_RIGHT) {
      board_redo(board);
    }

    break;
  }
}

static void handle_text_input_activation(UI_TextInput *ui_text_input, float mx, float my) {
  if (cursor_in_rect(mx, my, &ui_text_input->area.rect)) {
    ui_text_input->active = true;
    SDL_StartTextInput(SDL_GetKeyboardFocus());
  } else if (ui_text_input->active) {
    ui_text_input->active = false;
    SDL_StopTextInput(SDL_GetKeyboardFocus());
  }
}

static void handle_text_input_keys(SDL_Event *e, UI_TextInput *input, size_t max_len, void (*load_callback)(const char *, BoardState *), BoardState *board) {
  if (!input->active)
    return;

  SDL_Keycode kc = e->key.key;
  SDL_Keymod mod = SDL_GetModState();

  switch (kc) {
  case SDLK_BACKSPACE:
    if (input->length > 0) {
      input->length--;
      input->buf[input->length] = '\0';
      input->cache_valid        = false;
    }
    break;

  case SDLK_RETURN:
  case SDLK_KP_ENTER:
    if (input->length > 0 && load_callback) {
      load_callback(input->buf, board);
      input->active = false;
      SDL_StopTextInput(SDL_GetKeyboardFocus());
    }
    break;

  case SDLK_A:
    if (mod & SDL_KMOD_CTRL) {
      input->buf[0]      = '\0';
      input->length      = 0;
      input->cache_valid = false;
    }
    break;

  case SDLK_V:
    if (mod & SDL_KMOD_CTRL) {
      char *clip = SDL_GetClipboardText();

      if (clip) {
        size_t avail = max_len - input->length - 1;
        if (avail > 0) {
          SDL_strlcat(input->buf, clip, avail);
          input->length      = SDL_strlen(input->buf);
          input->cache_valid = false;
        }
        SDL_free(clip);
      }
    }
    break;
  }
}