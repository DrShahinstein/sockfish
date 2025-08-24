#include "board_render.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

void render_board_init(SDL_Renderer *renderer, BoardState *board) {
  const char *pieces = "rnbqkpRNBQKP";
  char path[256];

  for (const char *c = pieces; *c; ++c) {
    SDL_snprintf(path, sizeof path, PIECE_PATH, *c);
    SDL_Surface *surf = IMG_Load(path);

    if (!surf) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "IMG_Load failed: %s\n", SDL_GetError());
      continue;
    }

    board->tex[(int)*c] = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);

    if (!board->tex[(int)*c]) {
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
      if (board->drag.active && row == board->drag.row && col == board->drag.col) continue;

      char pc = board->board[row][col];

      if (pc && board->tex[(int)pc]) {
        SDL_FRect dst = {col * SQ, row * SQ, SQ, SQ};
        SDL_RenderTexture(renderer, board->tex[(int)pc], NULL, &dst);
      }
    }
  }

  if (board->drag.active) {
    char pc = board->board[board->drag.row][board->drag.col];
    float x, y;
    SDL_GetMouseState(&x, &y);

    if (pc && board->tex[(int)pc]) {
      SDL_FRect dst = {x - SQ / 2.0f, y - SQ / 2.0f, SQ, SQ};
      SDL_RenderTexture(renderer, board->tex[(int)pc], NULL, &dst);
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
      if (pc && board->tex[(int)pc]) {
        SDL_RenderTexture(renderer, board->tex[(int)pc], NULL, &dst);
      }
    }
  }
}

void render_board_cleanup(SDL_Renderer *renderer, BoardState *board) {
  (void)(renderer);

  for (int i = 0; i < 128; ++i) {
    if (board->tex[i]) {
      SDL_DestroyTexture(board->tex[i]);
      board->tex[i] = NULL;
    }
  }
}