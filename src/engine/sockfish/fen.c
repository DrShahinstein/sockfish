#include "fen.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

typedef struct {
  const char *text;
  size_t length;
} FenToken;

static bool next_token(const char **cursor, FenToken *token);
static bool token_equals(FenToken token, const char *expected);
static bool parse_piece_placement(FenToken token, SF_Fen *fen);
static bool parse_turn(FenToken token, Turn *turn);
static bool parse_castling(FenToken token, const char board[8][8], uint8_t *rights);
static bool parse_en_passant(FenToken token, const SF_Fen *fen, Square *ep_sq);
static bool parse_nonnegative_int(FenToken token, bool allow_zero, int *value);

bool sf_parse_fen(const char *fen, SF_Fen *out) {
  if (fen == NULL || out == NULL)
    return false;

  size_t length = strlen(fen);
  if (length == 0 || length >= SF_FEN_MAX_LENGTH)
    return false;

  const char *cursor = fen;
  FenToken fields[6];
  for (int i = 0; i < 6; ++i) {
    if (!next_token(&cursor, &fields[i]))
      return false;
  }

  FenToken extra;
  if (next_token(&cursor, &extra))
    return false;

  SF_Fen candidate       = {0};
  candidate.enpassant_sq = NO_ENPASSANT;

  if (!parse_piece_placement(fields[0], &candidate)                                              ||
      !parse_turn(fields[1], &candidate.turn)                                                    ||
      !parse_castling(fields[2], (const char (*)[8])candidate.board, &candidate.castling_rights) ||
      !parse_en_passant(fields[3], &candidate, &candidate.enpassant_sq)                          ||
      !parse_nonnegative_int(fields[4], true, &candidate.halfmove_clock)                         ||
      !parse_nonnegative_int(fields[5], false, &candidate.fullmove_number)) {
    return false;
  }

  if ((int)candidate.enpassant_sq != NO_ENPASSANT && candidate.halfmove_clock != 0)
    return false;

  *out = candidate;

  return true;
}




static bool next_token(const char **cursor, FenToken *token) {
  const char *start = *cursor;
  while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')
    ++start;

  if (*start == '\0') {
    *cursor = start;
    return false;
  }

  const char *end = start;
  while (*end != '\0' && *end != ' ' && *end != '\t' && *end != '\n' && *end != '\r')
    ++end;

  token->text   = start;
  token->length = (size_t)(end-start);
  *cursor       = end;

  return true;
}

static bool token_equals(FenToken token, const char *expected) {
  size_t expected_length = strlen(expected);
  return token.length == expected_length && memcmp(token.text, expected, expected_length) == 0;
}

static bool parse_piece_placement(FenToken token, SF_Fen *fen) {
  int row                 = 0;
  int col                 = 0;
  int kings[2]            = {0,0};
  int king_rows[2]        = {-1,-1};
  int king_cols[2]        = {-1,-1};
  int pawns[2]            = {0,0};
  int pieces[2]           = {0,0};
  bool previous_was_digit = false;

  for (size_t i = 0; i < token.length; ++i) {
    char piece = token.text[i];

    if (piece == '/') {
      if (col != 8 || row >= 7)
        return false;

      ++row;
      col = 0;
      previous_was_digit = false;
      continue;
    }

    if (piece >= '1' && piece <= '8') {
      if (previous_was_digit)
        return false;

      col += piece - '0';
      if (col > 8)
        return false;

      previous_was_digit = true;
      continue;
    }

    if (strchr("PNBRQKpnbrqk", piece) == NULL || col >= 8)
      return false;

    Turn color = (piece >= 'A' && piece <= 'Z') ? WHITE : BLACK;
    if (++pieces[color] > 16)
      return false;

    if (piece == 'K' || piece == 'k') {
      ++kings[color];
      king_rows[color] = row;
      king_cols[color] = col;
    }

    if (piece == 'P' || piece == 'p') {
      if (++pawns[color] > 8 || row == 0 || row == 7)
        return false;
    }

    fen->board[row][col++] = piece;
    previous_was_digit     = false;
  }

  if (row != 7 || col != 8 || kings[WHITE] != 1 || kings[BLACK] != 1) {
    return false;
  }

  int king_row_distance = king_rows[WHITE] - king_rows[BLACK];
  int king_col_distance = king_cols[WHITE] - king_cols[BLACK];

  if (king_row_distance < 0)
    king_row_distance = -king_row_distance;
  if (king_col_distance < 0)
    king_col_distance = -king_col_distance;

  return king_row_distance > 1 || king_col_distance > 1;
}

static bool parse_turn(FenToken token, Turn *turn) {
  if (token_equals(token, "w")) {
    *turn = WHITE;
    return true;
  }
  if (token_equals(token, "b")) {
    *turn = BLACK;
    return true;
  }

  return false;
}

static bool parse_castling(FenToken token, const char board[8][8], uint8_t *rights) {
  if (token_equals(token, "-")) {
    *rights = CASTLE_NONE;
    return true;
  }

  static const char order[] = "KQkq";
  int previous_order        = -1;
  uint8_t parsed            = CASTLE_NONE;

  if (token.length == 0 || token.length > 4)
    return false;

  for (size_t i = 0; i < token.length; ++i) {
    const char *ordered = strchr(order, token.text[i]);
    if (ordered == NULL)
      return false;

    int current_order = (int)(ordered-order);
    if (current_order <= previous_order)
      return false;

    parsed |= (uint8_t)(1U << current_order);
    previous_order = current_order;
  }

  if ((parsed & (CASTLE_WK | CASTLE_WQ)) && board[7][4] != 'K')
    return false;
  if ((parsed & CASTLE_WK) && board[7][7] != 'R')
    return false;
  if ((parsed & CASTLE_WQ) && board[7][0] != 'R')
    return false;
  if ((parsed & (CASTLE_BK | CASTLE_BQ)) && board[0][4] != 'k')
    return false;
  if ((parsed & CASTLE_BK) && board[0][7] != 'r')
    return false;
  if ((parsed & CASTLE_BQ) && board[0][0] != 'r')
    return false;

  *rights = parsed;

  return true;
}

static bool parse_en_passant(FenToken token, const SF_Fen *fen, Square *ep_sq) {
  if (token_equals(token, "-")) {
    *ep_sq = NO_ENPASSANT;
    return true;
  }

  char expected_rank = (fen->turn == WHITE) ? '6' : '3';
  if (token.length != 2   ||
      token.text[0] < 'a' || token.text[0] > 'h' ||
      token.text[1] != expected_rank) {
    return false;
  }

  int file = token.text[0] - 'a';
  int rank = token.text[1] - '1';
  int row  = 7 - rank;

  if (fen->board[row][file] != 0)
    return false;

  int pawn_row = row + ((fen->turn == WHITE) ? 1 : -1);
  char expected_pawn = (fen->turn == WHITE) ? 'p' : 'P';
  if (pawn_row < 0 || pawn_row >= 8 || fen->board[pawn_row][file] != expected_pawn)
    return false;

  int origin_row = pawn_row + ((fen->turn == WHITE) ? -2 : 2);
  if (origin_row < 0 || origin_row >= 8 || fen->board[origin_row][file] != 0)
    return false;

  *ep_sq = (Square)(rank * 8 + file);

  return true;
}

static bool parse_nonnegative_int(FenToken token, bool allow_zero, int *value) {
  if (token.length == 0)
    return false;

  int parsed = 0;
  for (size_t i = 0; i < token.length; ++i) {
    char ch = token.text[i];
    if (ch < '0' || ch > '9')
      return false;

    int digit = ch - '0';
    if (parsed > (INT_MAX - digit) / 10)
      return false;

    parsed = parsed*10 + digit;
  }

  if (!allow_zero && parsed == 0)
    return false;

  *value = parsed;
  
  return true;
}
