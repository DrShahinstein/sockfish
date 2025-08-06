#include "board.h"
#include <SDL3_image/SDL_image.h>
#include <ctype.h>
#include <stdio.h>

static const char *start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";

void load_fen(const char *fen, struct GameState *state) {
  int row = 0, col = 0;
  for (const char *p = fen; *p && row < 8; ++p) {
    if (isdigit((unsigned char)*p)) {
      col += *p - '0';
    } else if (*p == '/') {
      row++;
      col = 0;
    } else {
      if (col < 8)
        state->board[row][col++] = *p;
    }
  }
}

void load_piece_textures(SDL_Renderer *renderer, struct GameState *state) {
  const char *pieces = "rnbqkpRNBQKP";
  char path[256];

  for (const char *c = pieces; *c; ++c) {
    snprintf(path, sizeof path, PIECE_PATH, *c);
    SDL_Surface *surf = IMG_Load(path);

    if (!surf) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "IMG_Load failed: %s\n", SDL_GetError());
      continue;
    }

    state->tex[(int)*c] = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);

    if (!state->tex[(int)*c]) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "CreateTexture '%s' failed: %s\n", path, SDL_GetError());
    }
  }
}

void initialize_board(SDL_Renderer *renderer, struct GameState *state) {
  for (int r = 0; r < 8; ++r)
    for (int c = 0; c < 8; ++c)
      state->board[r][c] = 0;

  load_piece_textures(renderer, state);
  load_fen(start_fen, state);
}

void cleanup_textures(struct GameState *state) {
  for (int i = 0; i < 128; ++i) {
    if (state->tex[i]) {
      SDL_DestroyTexture(state->tex[i]);
      state->tex[i] = NULL;
    }
  }
}

void draw_board(SDL_Renderer *renderer, struct GameState *state) {
  for (int row = 0; row < 8; ++row) {
    for (int col = 0; col < 8; ++col) {
      SDL_FRect sq = {col * SQ, row * SQ, SQ, SQ};
      bool light = ((row + col) & 1) == 0;
      if (light) SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255);
      else       SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255);
      SDL_RenderFillRect(renderer, &sq);
    }
  }

  for (int row = 0; row < 8; ++row) {
    for (int col = 0; col < 8; ++col) {
      char pc = state->board[row][col];
      if (pc && state->tex[(int)pc]) {
        SDL_FRect dst = {col * SQ, row * SQ, SQ, SQ};
        SDL_RenderTexture(renderer, state->tex[(int)pc], NULL, &dst);
      }
    }
  }
}
