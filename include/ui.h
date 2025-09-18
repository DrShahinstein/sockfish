#pragma once

#include "engine.h"
#include "board.h"
#include "cursor.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define UI_WIDTH    250.0f
#define UI_PADDING  10.0f
#define UI_MIDDLE   (BOARD_SIZE + UI_WIDTH/2.0f)
#define UI_START_X  (BOARD_SIZE + UI_PADDING)
#define UI_START_Y  (UI_PADDING)
#define UI_FILLER_W (UI_WIDTH - UI_PADDING * 2)
#define ROBOTO "assets/Roboto-Regular.ttf"
#define JBMONO "assets/JetBrainsMonoNL-Regular.ttf" 
#define MAX_FEN 128
#define MAX_PGN 8192
#define FEN_PLACEHOLDER "Paste FEN here..."
#define PGN_PLACEHOLDER "Paste PGN here..."
#define FWHITE (SDL_Color){255,255,255,255}
#define FBLACK (SDL_Color){0,0,0,255}
#define FGRAY  (SDL_Color){150,150,150,255}

typedef struct {
  TTF_Font *roboto;
  TTF_Font *jbmono;
} FontMenu;

typedef struct {
  SDL_FRect rect;
  bool hovered;
} UI_Element;

enum InputType { FEN, PGN };
typedef struct {
  bool active;
  enum InputType type;
  UI_Element area;
  UI_Element btn;
  char *buf;
  char placeholder[32];
  size_t length;
} UI_TextInput;

typedef struct {
  bool engine_on;
  FontMenu fonts;
  UI_Element engine_toggler;
  UI_TextInput fen_loader;
  UI_TextInput pgn_loader;
  UI_Element separator;
  UI_Element turn_changer;
  UI_Element undo_btn;
  UI_Element redo_btn;
  UI_Element reset_btn;
} UI_State;

void ui_init(UI_State *ui);
void ui_handle_event(SDL_Event *e, UI_State *ui, BoardState *board);
void ui_render(SDL_Renderer *renderer, UI_State *ui, EngineWrapper *engine, BoardState *board);
void ui_destroy(UI_State *ui);
