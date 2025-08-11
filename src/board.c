#include "board.h"
#include <SDL3_image/SDL_image.h>
#include <ctype.h>
#include <stdio.h>

void load_fen(const char *fen, GameState *game) {
  int row = 0, col = 0;
  for (const char *p = fen; *p && row < 8; ++p) {
    if (isdigit((unsigned char)*p)) {
      col += *p - '0';
    } else if (*p == '/') {
      row++;
      col = 0;
    } else {
      if (col < 8) game->board[row][col++] = *p;
    }
  }
}

void load_piece_textures(SDL_Renderer *renderer, GameState *game) {
  const char *pieces = "rnbqkpRNBQKP";
  char path[256];

  for (const char *c = pieces; *c; ++c) {
    snprintf(path, sizeof path, PIECE_PATH, *c);
    SDL_Surface *surf = IMG_Load(path);

    if (!surf) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "IMG_Load failed: %s\n", SDL_GetError());
      continue;
    }

    game->tex[(int)*c] = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);

    if (!game->tex[(int)*c]) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "CreateTexture '%s' failed: %s\n", path, SDL_GetError());
    }
  }
}

void board_init(SDL_Renderer *renderer, GameState *game) {
  for (int r = 0; r < 8; ++r)
    for (int c = 0; c < 8; ++c)
      game->board[r][c] = 0;

  load_piece_textures(renderer, game);
  load_fen(START_FEN, game);
}

void load_board(const char *fen, GameState *game) {
  for (int r = 0; r < 8; ++r)
    for (int c = 0; c < 8; ++c)
      game->board[r][c] = 0;
  load_fen(fen, game);
}

void cleanup_textures(GameState *game) {
  for (int i = 0; i < 128; ++i) {
    if (game->tex[i]) {
      SDL_DestroyTexture(game->tex[i]);
      game->tex[i] = NULL;
    }
  }
}

void draw_board(SDL_Renderer *renderer, GameState *game) {
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
      if (game->drag.active && row == game->drag.row && col == game->drag.col) continue;

      char pc = game->board[row][col];

      if (pc && game->tex[(int)pc]) {
        SDL_FRect dst = {col * SQ, row * SQ, SQ, SQ};
        SDL_RenderTexture(renderer, game->tex[(int)pc], NULL, &dst);
      }
    }
  }

  if (game->drag.active) {
    char pc = game->board[game->drag.row][game->drag.col];
    float x, y;
    SDL_GetMouseState(&x, &y);

    if (pc && game->tex[(int)pc]) {
      SDL_FRect dst = {x - SQ / 2.0f, y - SQ / 2.0f, SQ, SQ};
      SDL_RenderTexture(renderer, game->tex[(int)pc], NULL, &dst);
    }
  }
}
