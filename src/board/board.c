#include "board.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

static uint8_t parse_castling(const char *str);
static bool validate_castling(const char *str);

void board_init(BoardState *board) {
  SDL_memset(board->board,         0, sizeof(board->board));
  SDL_memset(&board->promo,        0, sizeof(board->promo));
  SDL_memset(board->promo.choices, 0, sizeof(board->promo.choices));
  SDL_memset(&board->valid_moves,  0, sizeof(board->valid_moves));
  board->castling              = 0;
  board->turn                  = WHITE;
  board->ep_row                = NO_ENPASSANT;
  board->ep_col                = NO_ENPASSANT;
  board->drag.active           = false;
  board->drag.to_row           = -1;
  board->drag.to_col           = -1;
  board->drag.from_row         = -1;
  board->drag.from_col         = -1;
  board->promo.active          = false;
  board->promo.row             = -1;
  board->promo.col             = -1;
  board->promo.captured        = 0;
  board->selected_piece.active = false;
  board->selected_piece.row    = -1;
  board->selected_piece.col    = -1;
  board->undo_count            = 0;
  board->redo_count            = 0;

  load_fen(START_FEN, board);
}

void load_board(const char *fen, BoardState *board) {
  board->promo.active = false;
  board->undo_count   = 0;
  board->redo_count   = 0;
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

void board_save_history(BoardState *board, int from_row, int from_col, int to_row, int to_col) {
  if (board->undo_count >= MAX_HISTORY)
    return;

  board->redo_count = 0;

  BoardMoveHistory *h = &board->history[board->undo_count++];
  h->from_row         = from_row;
  h->from_col         = from_col;
  h->to_row           = to_row;
  h->to_col           = to_col;
  h->moving_piece     = board->board[from_row][from_col];
  h->castling         = board->castling;
  h->ep_row           = board->ep_row;
  h->ep_col           = board->ep_col;
  h->turn             = board->turn;
  h->promoted_piece   = 0;

  char moving_piece = board->board[from_row][from_col];

  bool en_passant   = (moving_piece == 'p' || moving_piece == 'P') && from_col != to_col && board->board[to_row][to_col] == 0 && to_row == board->ep_row && to_col == board->ep_col;
  if (en_passant) {
    int captured_row  = board->turn == WHITE ? (to_row + 1) : (to_row - 1);
    h->captured_piece = board->board[captured_row][to_col];
    h->captured_row   = captured_row;
    h->captured_col   = to_col;
  } else {
    h->captured_piece = board->board[to_row][to_col];
    h->captured_row   = to_row;
    h->captured_col   = to_col;
  }
}

void board_undo(BoardState *board) {
  if (board->undo_count <= 0)
    return;

  board->redo_count += 1;

  BoardMoveHistory *h                            = &board->history[--board->undo_count];
  board->turn                                    = h->turn;
  board->castling                                = h->castling;
  board->ep_row                                  = h->ep_row;
  board->ep_col                                  = h->ep_col;
  board->board[h->from_row][h->from_col]         = h->moving_piece;
  board->board[h->captured_row][h->captured_col] = h->captured_piece;
  board->selected_piece.active                   = false;

  char moving_piece = h->moving_piece;

  bool castling = (moving_piece == 'K' || moving_piece == 'k') && SDL_abs(h->from_col - h->to_col) == 2;
  if (castling) {
    int rook_from_col, rook_to_col;

    if (h->to_col > h->from_col) {
      rook_from_col = h->to_col - 1;
      rook_to_col   = 7;
    } else {
      rook_from_col = h->to_col + 1;
      rook_to_col   = 0;
    }
    
    char rook                                = (moving_piece == 'K') ? 'R' : 'r';
    board->board[h->from_row][rook_to_col]   = rook;
    board->board[h->from_row][rook_from_col] = 0;
  }

  bool en_passant = (moving_piece == 'p' || moving_piece == 'P') && h->from_col != h->to_col && h->captured_piece != 0 && h->captured_row != h->to_row;
  if (en_passant) {
    board->board[board->ep_row][board->ep_col] = 0;
  }
}

void board_redo(BoardState *board) {
  if (board->redo_count <= 0)
    return;

  board->redo_count -= 1;
  board->undo_count += 1;

  BoardMoveHistory *h                    = &board->history[board->undo_count - 1];
  char moving_piece                      = h->moving_piece;
  board->board[h->from_row][h->from_col] = 0;
  board->board[h->to_row][h->to_col]     = (h->promoted_piece) ? h->promoted_piece : moving_piece;;
  board->turn                            = (h->turn == WHITE) ? BLACK : WHITE;
  board->selected_piece.active           = false;

  bool castling = (moving_piece == 'K' || moving_piece == 'k') && SDL_abs(h->from_col - h->to_col) == 2;
  if (castling) {
    int rook_from_col, rook_to_col;

    if (h->to_col > h->from_col) {
      rook_from_col = 7;
      rook_to_col   = h->to_col - 1;
    } else {
      rook_from_col = 0;
      rook_to_col   = h->to_col + 1;
    }

    char rook                                = (moving_piece == 'K') ? 'R' : 'r';
    board->board[h->from_row][rook_to_col]   = rook;
    board->board[h->from_row][rook_from_col] = 0;
  }

  bool en_passant = (moving_piece == 'p' || moving_piece == 'P') && h->from_col != h->to_col && h->captured_piece != 0 && h->captured_row != h->to_row;
  if (en_passant) {
    board->board[h->captured_row][h->captured_col] = 0;
  }

  bool double_pawn_push = (moving_piece == 'p' || moving_piece == 'P') && SDL_abs(h->from_row - h->to_row) == 2;
  if (double_pawn_push) {
    board->ep_row = (h->from_row + h->to_row) / 2;
    board->ep_col = h->from_col;
  } else {
    board->ep_row = NO_ENPASSANT;
    board->ep_col = NO_ENPASSANT;
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