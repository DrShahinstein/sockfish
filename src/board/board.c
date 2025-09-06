#include "board.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

static uint8_t parse_castling(const char *str);
static bool validate_castling(const char *str);

void board_init(BoardState *board) {
  SDL_memset(board->board, 0, sizeof(board->board));
  SDL_memset(&board->promo, 0, sizeof(board->promo));
  SDL_memset(board->promo.choices, 0, sizeof(board->promo.choices));
  board->castling = 0;
  board->turn = WHITE;
  board->ep_row = NO_ENPASSANT;
  board->ep_col = NO_ENPASSANT;
  board->drag.active = false;
  board->drag.to_row = -1;
  board->drag.to_col = -1;
  board->drag.from_row = -1;
  board->drag.from_col = -1;
  board->promo.active = false;
  board->promo.row = -1;
  board->promo.col = -1;
  board->promo.captured = 0;

  load_fen(START_FEN, board);
}

void load_board(const char *fen, BoardState *board) {
  board->promo.active = false;
  SDL_memset(board->board, 0, sizeof(board->board));
  load_fen(fen, board);
}

void load_fen(const char *fen, BoardState *board) {
  SDL_memset(board->board, 0, sizeof(board->board));
  board->turn     = WHITE;
  board->castling = 0;
  board->ep_row   = NO_ENPASSANT;
  board->ep_col   = NO_ENPASSANT;

  char placement[256], active[2], castling[16], ep[3], halfmove[16], fullmove[16];
  int count = SDL_sscanf(fen, "%255s %1s %15s %2s %15s %15s",
    placement, active, castling, ep, halfmove, fullmove);

  if (count < 2) {
    SDL_Log("Invalid FEN: %s", fen);
    load_fen(START_FEN, board);
    return;
  }

  if (count >= 4 && SDL_strcmp(ep, "-") != 0) {
    board->ep_col = ep[0] - 'a';
    board->ep_row = 7 - (ep[1]-'1');  // convert rank to row (0-7)
  } else {
    board->ep_row = NO_ENPASSANT;
    board->ep_col = NO_ENPASSANT;
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
      SDL_strlcpy(castling, "KQkq", sizeof(castling));
    }
    board->castling = parse_castling(castling);
  } else {
    board->castling = parse_castling("KQkq");
  }

  int row = 0, col = 0;
  for (const char *p = placement; *p && row < 8; ++p) {
    if (SDL_isdigit((unsigned char)*p)) {
      col += *p - '0';
    } else if (*p == '/') {
      row++;
      col = 0;
    } else {
      if (col < 8) board->board[row][col++] = *p;
    }
  }
}

static uint8_t parse_castling(const char *str) {
  uint8_t rights = 0;
  if (SDL_strcmp(str, "-") == 0) return rights;
    
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
  if (SDL_strcmp(str, "-") == 0) return true;
    
  for (; *str; str++) {
    if (*str != 'K' && *str != 'Q' && *str != 'k' && *str != 'q') return false;
  }

  return true;
}