#include "board.h"
#include <SDL3_image/SDL_image.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static uint8_t parse_castling(const char *str) {
  uint8_t rights = 0;
  if (strcmp(str, "-") == 0) return rights;
    
  for (; *str; str++) {
    switch (*str) {
      case 'K': rights |= CASTLE_WK; break;
      case 'Q': rights |= CASTLE_WQ; break;
      case 'k': rights |= CASTLE_BK; break;
      case 'q': rights |= CASTLE_BQ; break;
    }
  }

  return rights;
}

static bool validate_castling(const char *str) {
  if (strcmp(str, "-") == 0) return true;
    
  for (; *str; str++) {
    if (*str != 'K' && *str != 'Q' && *str != 'k' && *str != 'q') return false;
  }

  return true;
}

void load_fen(const char * fen, BoardState * board) {
  memset(board->board, 0, sizeof(board->board));
  board->castling = 0;
  board->turn = WHITE;
  board->ep_row = -1;
  board->ep_col = -1;

  char placement[256], active[2], castling[16], ep[3], halfmove[16], fullmove[16];
  int count = sscanf(fen, "%255s %1s %15s %2s %15s %15s",
    placement, active, castling, ep, halfmove, fullmove);

  if (count < 2) {
    SDL_Log("Invalid FEN: %s", fen);
    load_fen(START_FEN, board);
    return;
  }

  if (count >= 4 && SDL_strcmp(ep, "-") != 0) {
    board->ep_col = ep[0] - 'a';
    board->ep_row = 7 - (ep[1]-'1');  // convert rank to row (0-7)
  }

  if (active[0] == 'w' || active[0] == 'W') {
    board->turn = WHITE;
  } else if (active[0] == 'b' || active[0] == 'B') {
    board->turn = BLACK;
  } else {
    SDL_Log("Invalid active color in FEN: %c", active[0]);
  }

  if (count >= 3) {
    if (!validate_castling(castling)) {
      SDL_Log("Invalid castling rights in FEN: %s", castling);
      strcpy(castling, "KQkq");
    }
    board->castling = parse_castling(castling);
  } else {
    board->castling = parse_castling("KQkq");
  }

  int row = 0, col = 0;
  for (const char *p = placement; *p && row < 8; ++p) {
    if (isdigit((unsigned char) * p)) {
      col += * p - '0';
    } else if ( * p == '/') {
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
  memset(board->board, 0, sizeof(board->board));
  memset(board->tex, 0, sizeof(board->tex));
  memset(&board->promo, 0, sizeof(board->promo));
  board->castling = 0;
  board->promo.active = false;

  load_piece_textures(renderer, board);
  load_fen(START_FEN, board);
}

void load_board(const char *fen, BoardState *board) {
  board->promo.active = false;
  memset(board->board, 0, sizeof(board->board));
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
