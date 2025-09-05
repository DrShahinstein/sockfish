#include "special_moves.h"

void update_castling_rights(BoardState *board, char moving_piece, Move move) {
  Square from_sq = move_from(move);
  int fr = square_to_row(from_sq);
  int fc = square_to_col(from_sq);

  if (moving_piece == 'K')      board->castling &= ~(CASTLE_WK | CASTLE_WQ);
  else if (moving_piece == 'k') board->castling &= ~(CASTLE_BK | CASTLE_BQ);

  else if (moving_piece == 'R') {
    if     (fr == 7 && fc == 0)  board->castling &= ~CASTLE_WQ;
    else if (fr == 7 && fc == 7) board->castling &= ~CASTLE_WK;
  }
  
  else if (moving_piece == 'r') {
    if      (fr == 0 && fc == 0) board->castling &= ~CASTLE_BQ;
    else if (fr == 0 && fc == 7) board->castling &= ~CASTLE_BK;
  }
}

bool is_castling_move(BoardState *board, Move move) {
  Square from_sq = move_from(move);
  Square to_sq   = move_to(move);
  int fr = square_to_row(from_sq);
  int fc = square_to_col(from_sq);
  int tr = square_to_row(to_sq);
  int tc = square_to_col(to_sq);

  char piece = board->board[fr][fc];

  if ((piece != 'K' && piece != 'k') || (fr != tr) || SDL_abs(fc - tc) != 2) {
    return false;
  }

  bool kingside = (tc > fc);
  int rook_col  = kingside ? 7:0;
  char rook     = (piece == 'K') ? 'R':'r';

  if (board->board[fr][rook_col] != rook) {
    return false;
  }

  uint8_t required_right;
  if (piece == 'K') {
    required_right = kingside ? CASTLE_WK : CASTLE_WQ;
  } else {
    required_right = kingside ? CASTLE_BK : CASTLE_BQ;
  }

  return (board->castling & required_right) != 0;
}

void perform_castling(BoardState *board, Move move) {
  Square from_sq = move_from(move);
  Square to_sq   = move_to(move);
  int fr = square_to_row(from_sq);
  int fc = square_to_col(from_sq);
  int tr = square_to_row(to_sq);
  int tc = square_to_col(to_sq);

  char piece      = board->board[fr][fc];
  bool kingside   = (tc > fc);
  int rook_fr_col = kingside ? 7 : 0;
  int rook_to_col = kingside ? tc-1 : tc+1;

  board->board[tr][tc] = piece;
  board->board[fr][fc] = 0;

  char rook = board->board[fr][rook_fr_col];
  board->board[fr][rook_to_col] = rook;
  board->board[fr][rook_fr_col] = 0;

  if (piece == 'K') {
    board->castling &= ~(CASTLE_WK | CASTLE_WQ);
  } else {
    board->castling &= ~(CASTLE_BK | CASTLE_BQ);
  }
}

bool is_en_passant_capture(BoardState *board, Move move) {
  Square from_sq = move_from(move);
  Square to_sq   = move_to(move);
  int fr = square_to_row(from_sq);
  int fc = square_to_col(from_sq);
  int tr = square_to_row(to_sq);
  int tc = square_to_col(to_sq);

  char piece = board->board[fr][fc];
  
  if ((piece != 'p' && piece != 'P') || fc == tc || board->board[tr][tc] != 0) {
    return false;
  }
  
  return (tr == board->ep_row && tc == board->ep_col);
}

void update_enpassant_rights(BoardState *board, char moving_piece) {
  if (moving_piece != 'p' && moving_piece != 'P') {
    board->ep_row = -1;
    board->ep_col = -1;
  }
}