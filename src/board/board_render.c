#include "board_render.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

SDL_Texture *tex[128];

void render_board_init(SDL_Renderer *renderer) {
  const char *pieces = "rnbqkpRNBQKP";
  char path[256];

  for (const char *c = pieces; *c; ++c) {
    SDL_snprintf(path, sizeof path, PIECE_PATH, *c);
    SDL_Surface *surf = IMG_Load(path);

    if (!surf) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "IMG_Load failed: %s\n", SDL_GetError());
      continue;
    }

    tex[(int)*c] = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);

    if (!tex[(int)*c]) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "CreateTexture '%s' failed: %s\n", path, SDL_GetError());
    }
  }
}

void render_board(SDL_Renderer *renderer, BoardState *board) {
  if (!renderer || !board) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "render_board: invalid parameters\n");
    return;
  }

  for (int row = 0; row < 8; ++row) {
    for (int col = 0; col < 8; ++col) {
      SDL_FRect sq = {col * SQ, row * SQ, SQ, SQ};
      bool light = ((row + col) & 1) == 0;
      if (light) SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255);
      else SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255);
      SDL_RenderFillRect(renderer, &sq);
    }
  }

  for (int row = 0; row < 8; ++row) {
    for (int col = 0; col < 8; ++col) {
      if (board->drag.active && row == board->drag.to_row && col == board->drag.to_col) continue;

      char pc = board->board[row][col];

      if (pc && tex[(int)pc]) {
        SDL_FRect dst = {col * SQ, row * SQ, SQ, SQ};
        SDL_RenderTexture(renderer, tex[(int)pc], NULL, &dst);
      }
    }
  }

  if (board->drag.active) {
    char pc = board->board[board->drag.to_row][board->drag.to_col];
    float x, y;
    SDL_GetMouseState(&x, &y);

    if (pc && tex[(int)pc]) {
      SDL_FRect dst = {x - SQ / 2.0f, y - SQ / 2.0f, SQ, SQ};
      SDL_RenderTexture(renderer, tex[(int)pc], NULL, &dst);
    }
  }

  if (board->promo.active) {
    for (int i = 0; i < 4; ++i) {
      float menu_x = board->promo.col * SQ;
      float menu_y = board->turn == WHITE ? (i*SQ) : ((7-i) * SQ);

      SDL_FRect dst = {menu_x, menu_y, SQ, SQ};
      SDL_SetRenderDrawColor(renderer, 169, 169, 169, 255);
      SDL_RenderFillRect(renderer, &dst);

      char pc = board->promo.choices[i];
      if (pc && tex[(int)pc]) {
        SDL_RenderTexture(renderer, tex[(int)pc], NULL, &dst);
      }
    }
  }

  if (board->selected_piece.active) {
    char pc = board->board[board->selected_piece.row][board->selected_piece.col];

    if (pc && tex[(int)pc]) {
      SelectedPiece s    = board->selected_piece;
      Square selected_sq = rowcol_to_sq_for_engine(s.row, s.col);

      for (int i = 0; i < board->valid_moves.count; ++i) {
        Move mv = board->valid_moves.moves[i];

        if (move_from(mv) != selected_sq) continue;

        Square to = move_to(mv);
        int row   = 7 - square_to_row(to);
        int col   = square_to_col(to);
        
        /* == Rendering Dots On Valid Squares == */
        float radius  = 8.0f;
        float outer_r = radius + 2.5f;
        float centerX = col * SQ + (SQ / 2.0f);
        float centerY = row * SQ + (SQ / 2.0f);

        SDL_SetRenderDrawColor(renderer, 150, 20, 20, 255);

        int r_out = (int)SDL_ceilf(outer_r);
        for (int dy = -r_out; dy <= r_out; ++dy) {
          float dyf = (float)dy;
          float dx = SDL_sqrtf(SDL_max(0.0f, outer_r*outer_r - dyf * dyf));
          SDL_FRect span = {centerX - dx, centerY + dyf - 0.5f, dx * 2.0f, 1.0f};
          SDL_RenderFillRect(renderer, &span);
        }
      } 
    }
  }
}

void render_board_cleanup(void) {
  for (int i = 0; i < 128; ++i) {
    if (tex[i]) {
      SDL_DestroyTexture(tex[i]);
      tex[i] = NULL;
    }
  }
}