#include "ui.h"
#include "ui_helpers.h"
#include <SDL3/SDL.h>

void ui_render(SDL_Renderer *r, UI_State *ui, EngineWrapper *engine, BoardState *board) {
  // Panel
  SDL_FRect panel = {BOARD_SIZE, 0, UI_WIDTH, BOARD_SIZE};
  SDL_SetRenderDrawColor(r, 40, 44, 52, 255);
  SDL_RenderFillRect(r, &panel);

  // Engine Toggler
  SDL_FRect tog = ui->engine_toggler.rect;
  SDL_SetRenderDrawColor(r, ui->engine_on ? 100 : 200, ui->engine_on ? 200 : 100, 100, 255);
  SDL_RenderFillRect(r, &tog);
  draw_text(r, ui->fonts.roboto, ui->engine_on ? "Engine: ON" : "Engine: OFF", FWHITE, tog.x + tog.w + 10, tog.y + 5);

  /* --- FEN Input Area --- */
  render_text_input(r, &ui->fen_loader, &ui->fonts);

  SDL_FRect fenbtn = ui->fen_loader.btn.rect;
  SDL_SetRenderDrawColor(r, ui->fen_loader.btn.hovered ? 170 : 200, 200, 200, 255);
  SDL_RenderFillRect(r, &fenbtn);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &fenbtn);
  TTF_SetFontSize(ui->fonts.roboto, 15);
  draw_text_centered(r, ui->fonts.roboto, "Load Fen", FBLACK, fenbtn);
  TTF_SetFontSize(ui->fonts.roboto, 16);
  
  /* --- PGN Input Area --- */
  render_text_input(r, &ui->pgn_loader, &ui->fonts);

  SDL_FRect pgnbtn = ui->pgn_loader.btn.rect;
  SDL_SetRenderDrawColor(r, ui->pgn_loader.btn.hovered ? 170 : 200, 200, 200, 255);
  SDL_RenderFillRect(r, &pgnbtn);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &pgnbtn);
  TTF_SetFontSize(ui->fonts.roboto, 15);
  draw_text_centered(r, ui->fonts.roboto, "Load Pgn", FBLACK, pgnbtn);
  TTF_SetFontSize(ui->fonts.roboto, 16);

  /* --- Sockfish Engine --- */
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
    draw_text(r, ui->fonts.roboto, white ? "White to play" : "Black to play", FWHITE, turn_changer.x + turn_changer.w + 10, turn_changer.y + 5);

    engine_req_search(engine, board);
    
    Move sf_best;
    bool search_thr_active;

    SDL_LockMutex(engine->mtx);
    sf_best           = engine->ctx.best;
    search_thr_active = engine->thr_working;
    SDL_UnlockMutex(engine->mtx);

    float x = ui->turn_changer.rect.x;
    float y = ui->turn_changer.rect.y + 50;

    if (search_thr_active) draw_text(r, ui->fonts.roboto, "Thinking...", FWHITE, x, y);
    else {
      char from_alg[3], to_alg[3];
      Square sf_from = move_from(sf_best);
      Square sf_to   = move_to(sf_best);
      sq_to_alg(sf_from, from_alg);
      sq_to_alg(sf_to,   to_alg);

      char move_str[16];
      SDL_snprintf(move_str, sizeof(move_str), "BEST: %s%s", from_alg, to_alg);

      draw_text(r, ui->fonts.roboto, move_str, FWHITE, x, y);
    }
  }

  // Undo Button
  SDL_FRect undobtn = ui->undo_btn.rect;
  SDL_SetRenderDrawColor(r, ui->undo_btn.hovered ? 170 : 200, 200, 200, 255);
  SDL_RenderFillRect(r, &undobtn);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &undobtn);
  TTF_SetFontSize(ui->fonts.roboto, 15);
  draw_text_centered(r, ui->fonts.roboto, "Undo", FBLACK, ui->undo_btn.rect);

  // Redo Button
  SDL_FRect redobtn = ui->redo_btn.rect;
  SDL_SetRenderDrawColor(r, ui->redo_btn.hovered ? 170 : 200, 200, 200, 255);
  SDL_RenderFillRect(r, &redobtn);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &redobtn);
  draw_text_centered(r, ui->fonts.roboto, "Redo", FBLACK, ui->redo_btn.rect);

  // Reset Button
  SDL_FRect resetbtn = ui->reset_btn.rect; 
  SDL_SetRenderDrawColor(r, ui->reset_btn.hovered ? 170 : 200, 200, 200, 255);
  SDL_RenderFillRect(r, &resetbtn);
  SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
  SDL_RenderRect(r, &resetbtn);
  draw_text_centered(r, ui->fonts.roboto, "Reset", FBLACK, ui->reset_btn.rect);
  TTF_SetFontSize(ui->fonts.roboto, 16);
}