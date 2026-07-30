#pragma once

#include "sockfish.h"

#define SF_FEN_MAX_LENGTH 128
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

typedef struct {
  char board[8][8];
  Turn turn;
  uint8_t castling_rights;
  Square enpassant_sq;
  int halfmove_clock;
  int fullmove_number;
} SF_Fen;

bool sf_parse_fen(const char *fen, SF_Fen *out);
