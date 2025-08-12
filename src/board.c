#include "board.h"
#include <SDL3_image/SDL_image.h>
#include <ctype.h>
#include <stdio.h>

void load_fen(const char *fen, BoardState *board) {
  int row = 0, col = 0;
  for (const char *p = fen; *p && row < 8; ++p) {
    if (isdigit((unsigned char)*p)) {
      col += *p - '0';
    } else if (*p == '/') {
      row++;
      col = 0;
    } else {
      if (col < 8) board->board[row][col++] = *p;
    }
  }
}

void load_piece_textures(SDL_Renderer *renderer, BoardState *board) {
  const char *pieces = "rnbqkpRNBQKP";
  char path[256];

  for (const char *c = pieces; *c; ++c) {
    snprintf(path, sizeof path, PIECE_PATH, *c);
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

void board_init(SDL_Renderer *renderer, BoardState *board) {
  for (int r = 0; r < 8; ++r)
    for (int c = 0; c < 8; ++c)
      board->board[r][c] = 0;

  load_piece_textures(renderer, board);
  load_fen(START_FEN, board);
}

void load_board(const char *fen, BoardState *board) {
  for (int r = 0; r < 8; ++r)
    for (int c = 0; c < 8; ++c)
      board->board[r][c] = 0;
  load_fen(fen, board);
}

void cleanup_textures(BoardState *board) {
  for (int i = 0; i < 128; ++i) {
    if (board->tex[i]) {
      SDL_DestroyTexture(board->tex[i]);
      board->tex[i] = NULL;
    }
  }
}

void draw_board(SDL_Renderer *renderer, BoardState *board) {
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
}
