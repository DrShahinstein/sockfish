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
#define NOTO   "assets/NotoSans-Regular.ttf"
#define MAX_FEN 128
#define MAX_PGN 8192
#define FEN_PLACEHOLDER "Paste FEN here..."
#define PGN_PLACEHOLDER "Paste PGN here..."
#define MAX_INFO_LENGTH 64
#define ARROW_COLORS_COUNT 4
#define FWHITE  (SDL_Color){255,255,255,255}
#define FBLACK  (SDL_Color){0,0,0,255}
#define FGRAY   (SDL_Color){150,150,150,255}
#define FYELLOW (SDL_Color){224,224,76,255}

static const SDL_FColor DEFAULT_ARROW_COLOR = {214.0f/255.0f, 58.0f/255.0f, 40.0f/255.0f, 0.8f};

typedef struct {
  TTF_Font *noto15;
  TTF_Font *roboto16;
  TTF_Font *roboto15;
  TTF_Font *jbmono14;
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
  char **wrap_lines;
  int wrap_line_count;
  int cached_line_count;
  size_t cached_text_hash;
  float cached_wrap_width;
  bool cache_valid;
} UI_TextInput;

typedef struct {
  SDL_FRect rect;
  bool hovered;
  SDL_FColor *colors;
  int color_idx;
} UI_ColorPicker;

typedef struct {
  bool engine_on;
  FontMenu fonts;
  UI_Element engine_toggler;
  UI_TextInput fen_loader;
  UI_TextInput pgn_loader;
  UI_Element info_box;
  UI_Element separator;
  UI_Element turn_changer;
  UI_ColorPicker arrow_changer;
  UI_Element undo_btn;
  UI_Element redo_btn;
  UI_Element reset_btn;
} UI_State;

void ui_init(UI_State *ui);
void ui_handle_event(SDL_Event *e, UI_State *ui, BoardState *board);
void ui_destroy(UI_State *ui);

// ui_helpers.c
void draw_text(SDL_Renderer *r, TTF_Font *font, const char *text, SDL_Color color, float x, float y);
void draw_text_centered(SDL_Renderer *r, TTF_Font *font, const char *text, SDL_Color color, SDL_FRect rect);
int wrap_text_into_lines(TTF_Font *font, const char *text, float max_w, char **lines, int max_lines, int max_buf_size);
void render_text_input(SDL_Renderer *r, UI_TextInput *ui_text_input, FontMenu *fonts);

// info.c
void info_system_init(void);
void info_system_cleanup(void);
void ui_set_info(const char *msg, ...);
const char *get_info_message(void);