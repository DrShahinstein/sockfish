#include "ui.h"
#include "ui_helpers.h"
#include <SDL3/SDL.h>

void ui_render(SDL_Renderer *r, UI_State *ui, EngineWrapper *engine, BoardState *board) {
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
    bool white = board->turn == WHITE;
    if (white) SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    else SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
    SDL_RenderFillRect(r, &turn_changer);
    draw_text(r, ui->font, white ? "White to play" : "Black to play", FWHITE, turn_changer.x + turn_changer.w + 10, turn_changer.y + 5);

    engine_req_search(engine, board);
    
    Move sf_best;
    bool thinking;

    SDL_LockMutex(engine->mtx);
    sf_best  = engine->ctx.best;
    thinking = engine->ctx.thinking;
    SDL_UnlockMutex(engine->mtx);

    float x = ui->turn_changer.rect.x;
    float y = ui->turn_changer.rect.y + 50;

    if (thinking) draw_text(r, ui->font, "Thinking...", FWHITE, x, y);
    else {
      char from_alg[3], to_alg[3];
      Square sf_from = move_from(sf_best);
      Square sf_to   = move_to(sf_best);
      sq_to_alg(sf_from, from_alg);
      sq_to_alg(sf_to,   to_alg);

      char move_str[16];
      SDL_snprintf(move_str, sizeof(move_str), "BEST: %s%s", from_alg, to_alg);

      draw_text(r, ui->font, move_str, FWHITE, x, y);
    }
  }
}