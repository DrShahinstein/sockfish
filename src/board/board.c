#include "board.h"
#include "sockfish/sockfish.h"
#include "sockfish/evaluation.h"          /* is_insufficient_material() */
#include "sockfish/fen.h"
#include "sockfish/move_helper.h"         /* king_in_check() */
#include "sockfish/transposition_table.h" /* tt_clear() */
#include "engine.h"                       /* make_bitboards_from_charboard() */
#include "ui.h"                           /* ui_set_info() */
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#define MAX_PGN_MOVE_LENGTH 16

static bool threefold_repetition(const BoardState *board);
static void board_update_game_result(BoardState *board);
static void end_game(BoardState *board, GameResult result, const char *message);
static void adjust_promoting_pawn(BoardState *tmp_b, char promote, Turn T, int tr, int tc);
static void adjust_castling_rook(BoardState *tmp_b, int king_to_col, int row);
static void adjust_castling_flags(uint8_t *c, char moved, int fr, int fc, char captured, int tr, int tc);
static void adjust_enpassant(int *ep_row, int *ep_col, char p, int fr, int fc, int tr);

void board_init(BoardState *board) {
  SDL_memset(board->board,             0, sizeof(board->board));
  SDL_memset(&board->promo,            0, sizeof(board->promo));
  SDL_memset(board->promo.choices,     0, sizeof(board->promo.choices));
  SDL_memset(&board->valid_moves,      0, sizeof(board->valid_moves));
  board->position_hash               = 0;
  board->castling                    = 0;
  board->turn                        = WHITE;
  board->ep_row                      = NO_ENPASSANT;
  board->ep_col                      = NO_ENPASSANT;
  board->halfmove_clock              = 0;
  board->game_result                 = GAME_ONGOING;
  board->flipped                     = false;
  board->drag.active                 = false;
  board->drag.to_row                 = -1;
  board->drag.to_col                 = -1;
  board->drag.from_row               = -1;
  board->drag.from_col               = -1;
  board->promo.active                = false;
  board->promo.row                   = -1;
  board->promo.col                   = -1;
  board->promo.captured              = 0;
  board->selected_piece.active       = false;
  board->selected_piece.row          = -1;
  board->selected_piece.col          = -1;
  board->undo_count                  = 0;
  board->redo_count                  = 0;
  board->should_update_valid_moves   = true;
  board->king.in_check               = false;
  board->king.color                  = 0;
  board->king.row                    = -1;
  board->king.col                    = -1;
  board->annotations.drawing_arrow   = false;
  board->annotations.arrow_count     = 0;
  board->annotations.highlight_count = 0;
  board->annotations.arrow_color     = DEFAULT_ARROW_COLOR;
  board->annotations.highlight_color = DEFAULT_HIGHLIGHT_COLOR;

  load_fen(START_FEN, board);
}

void board_update_position_hash(BoardState *board) {
  BitboardSet bbset = make_bitboards_from_charboard((const char (*)[8])board->board);
  Square ep_sq      = (board->ep_row >= 0 && board->ep_col >= 0) ? rowcol_to_sq(board->ep_row, board->ep_col) : NO_ENPASSANT;
  
  SF_Context temp_ctx;
  temp_ctx.bitboard_set    = bbset;
  temp_ctx.search_color    = board->turn;
  temp_ctx.castling_rights = board->castling;
  temp_ctx.enpassant_sq    = ep_sq;

  sf_init_hash_key(&temp_ctx);
  
  board->position_hash = temp_ctx.hash_key;

  if (board->undo_count >= 0 && board->undo_count <= MAX_HISTORY) {
    board->hash_history[board->undo_count] = board->position_hash;
  }

  board_update_game_result(board);
}

void board_update_valid_moves(BoardState *b) {
  if (!b->should_update_valid_moves)
    return;

  bool ep_valid = b->ep_row >= 0 && b->ep_col >= 0;

  BitboardSet bbset = make_bitboards_from_charboard((const char (*)[8]) b->board);
  Square en_passant = ep_valid ? rowcol_to_sq(b->ep_row, b->ep_col) : NO_ENPASSANT;
  SF_Context ctx    = create_sf_ctx(&bbset, b->turn, b->castling, en_passant);

  MoveList valids = sf_generate_moves(&ctx);

  b->valid_moves = valids;
  b->should_update_valid_moves = false;
}

void board_update_king_in_check(BoardState *b) {
  bool in_check = false;
  int king_row  = -1;
  int king_col  = -1;

  BitboardSet bbset = make_bitboards_from_charboard((const char (*)[8]) b->board);

  if (king_in_check(&bbset, b->turn)) {
    in_check = true;

    uint64_t bb = bbset.kings[b->turn];
    Square sq   = GET_LSB(bb);           /* since king is always an only piece */
    king_row    = square_to_row(sq);
    king_col    = square_to_col(sq);
  }

  b->king.in_check = in_check;
  b->king.color    = b->turn;
  b->king.row      = king_row;
  b->king.col      = king_col;
}

void load_fen(const char *fen, BoardState *board) {
  SF_Fen parsed;
  if (!sf_parse_fen(fen, &parsed)) {
    ui_set_info("Can't load invalid FEN.");
    return;
  }

  SDL_memset(board->board,           0,        sizeof(board->board));
  SDL_memset(board->history,         0,        sizeof(board->history));
  SDL_memset(board->hash_history,    0,        sizeof(board->hash_history));
  SDL_memcpy(board->board,       parsed.board, sizeof(board->board));
  board->turn                  = parsed.turn;
  board->castling              = parsed.castling_rights;
  board->halfmove_clock        = parsed.halfmove_clock;
  board->game_result           = GAME_ONGOING;
  board->promo.active          = false;
  board->undo_count            = 0;
  board->redo_count            = 0;
  board->selected_piece.active = false;
  board->drag.active           = false;
  board->king.in_check         = false;
  board->king.color            = 0;
  board->king.row              = -1;
  board->king.col              = -1;

  if ((int)parsed.enpassant_sq != NO_ENPASSANT) {
    board->ep_row = square_to_row(parsed.enpassant_sq);
    board->ep_col = square_to_col(parsed.enpassant_sq);
  } else {
    board->ep_row = NO_ENPASSANT;
    board->ep_col = NO_ENPASSANT;
  }

  clear_annotations(&board->annotations);
  board->should_update_valid_moves = true;
  board_update_position_hash(board);
  board_update_king_in_check(board);
}

void load_pgn(const char *pgn, BoardState *board) {
  load_fen(START_FEN, board);

  const char *ptr = pgn;

  /* Skip PGN Headers [...] */
  while (*ptr != '\0') {
    while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r') ptr++;

    if (*ptr == '[') {
      while (*ptr != '\0' && *ptr != '\n' && *ptr != '\r') ptr++;
      continue;
    }

    break;
  }

  ptr = SDL_strstr(ptr, "1.");

  if (!ptr) {
    ui_set_info("Invalid pgn cannot load.");
    return;
  }

  ptr += 2;

  /* Build Move History in a Fake Board */
  BoardState tmp_b  = *board;
  BitboardSet bbset = make_bitboards_from_charboard((const char (*)[8]) tmp_b.board);
  SF_Context ctx    = create_sf_ctx(&bbset, WHITE, CASTLE_ALL, NO_ENPASSANT);

  while (*ptr != '\0') {
    while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r') ptr++;

    if (*ptr == '\0')
      break;

    /* Skip PGN Comments {...} */
    if (*ptr == '{') {
      while (*ptr != '}' && *ptr != '\0') ptr++;
      if    (*ptr == '}'                ) ptr++;
      continue;
    }

    bool is_game_result = SDL_strncmp(ptr, "1-0", 3)     == 0 ||
                          SDL_strncmp(ptr, "0-1", 3)     == 0 ||
                          SDL_strncmp(ptr, "1/2-1/2", 7) == 0;

    if (is_game_result)
      break;

    if (SDL_isdigit(*ptr)) {
      const char *ptr_move_number = ptr;

      while (SDL_isdigit(*ptr_move_number))
        ptr_move_number++;

      if (*ptr_move_number == '.' && *(ptr_move_number + 1) == '.' && *(ptr_move_number + 2) == '.') {
        ptr = ptr_move_number + 3; // skip "..." seen in annotated pgns
        continue;
      }

      if (*ptr_move_number == '.') {
        ptr = ptr_move_number + 1;
        continue;
      }
    }

    const char *move_start = ptr;
    while (*ptr != ' ' && *ptr != '\t' && *ptr != '\0' && *ptr != '\n' && *ptr != '\r')
      ptr++;

    size_t move_length = (size_t)(ptr - move_start);
    if (move_length >= MAX_PGN_MOVE_LENGTH) {
      ui_set_info("PGN move token is too long.");
      return;
    }

    char pgn_move[MAX_PGN_MOVE_LENGTH];
    SDL_memcpy(pgn_move, move_start, move_length);
    pgn_move[move_length] = '\0';

    if (pgn_move[0] != '\0') {
      if (tmp_b.redo_count >= MAX_HISTORY)
        break;

      Square ep;
      bool ep_invalid = tmp_b.ep_row == NO_ENPASSANT || tmp_b.ep_col == NO_ENPASSANT;

      /* Refresh Context */
      bbset = make_bitboards_from_charboard((const char (*)[8]) tmp_b.board);
      ep    = ep_invalid ? NO_ENPASSANT : rowcol_to_sq(tmp_b.ep_row, tmp_b.ep_col);
      ctx   = create_sf_ctx(&bbset, tmp_b.turn, tmp_b.castling, ep);

      char promote = -1;
      int  fr=-1, tr=-1,
           fc=-1, tc=-1;
      parse_pgn_move(pgn_move, &ctx, tmp_b.board, &promote, &fr, &fc, &tr, &tc);

      bool parsing_failed = (fr < 0 || fr > 7) ||
                            (fc < 0 || fc > 7) ||
                            (tr < 0 || tr > 7) ||
                            (tc < 0 || tc > 7);

      if (parsing_failed) {
        ui_set_info("Failed to parse pgn move.");
        return;
      }

      /* Save Before Applying */
      board_save_history(&tmp_b, fr, fc, tr, tc, tmp_b.redo_count);

      char moved_piece              = tmp_b.board[fr][fc];
      const BoardMoveHistory *saved = &tmp_b.history[tmp_b.redo_count];
      bool irreversible             = moved_piece == 'P' || moved_piece == 'p' || saved->captured_piece != 0;
      tmp_b.halfmove_clock          = next_halfmove_clock(tmp_b.halfmove_clock, irreversible);

      Turn t           = tmp_b.turn;
      bool is_castling = (moved_piece == 'K' || moved_piece == 'k') && SDL_abs(fc - tc) == 2;

      /* Apply Incoming Move */
      tmp_b.redo_count   += 1;
      tmp_b.turn          = ctx.search_color;
      tmp_b.board[tr][tc] = moved_piece;
      tmp_b.board[fr][fc] = 0;

      bool en_passant = saved->captured_piece != 0 && saved->captured_row != tr;
      if (en_passant)
        tmp_b.board[saved->captured_row][saved->captured_col] = 0;

      if (promote != -1)
        adjust_promoting_pawn(&tmp_b, promote, t, tr, tc);

      if (is_castling)
        adjust_castling_rook(&tmp_b, tc, tr);

      adjust_enpassant(&tmp_b.ep_row, &tmp_b.ep_col, moved_piece, fr, fc, tr);
      adjust_castling_flags(&tmp_b.castling, moved_piece, fr, fc, saved->captured_piece, tr, tc);
    }
  }

  /* Copy History to Main Board */
  SDL_memcpy(board->history, &tmp_b.history, sizeof(board->history));
  board->redo_count = tmp_b.redo_count;

  board_update_position_hash(board);

  ui_set_info("Pgn loaded successfully.");
}

void board_save_history(BoardState *board, int from_row, int from_col, int to_row, int to_col, int history_index) {
  char moving_piece = board->board[from_row][from_col];

  BoardMoveHistory *h = &board->history[history_index];
  h->from_row         = from_row;
  h->from_col         = from_col;
  h->to_row           = to_row;
  h->to_col           = to_col;
  h->moving_piece     = moving_piece;
  h->castling         = board->castling;
  h->ep_row           = board->ep_row;
  h->ep_col           = board->ep_col;
  h->halfmove_clock   = board->halfmove_clock;
  h->turn             = board->turn;
  h->promoted_piece   = 0;

  bool en_passant = (moving_piece == 'p' || moving_piece == 'P') && from_col != to_col && board->board[to_row][to_col] == 0 && to_row == board->ep_row && to_col == board->ep_col;
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

  bool cancelling_promotion = board->promo.active;

  board->redo_count += 1;

  BoardMoveHistory *h                            = &board->history[--board->undo_count];
  board->turn                                    = h->turn;
  board->castling                                = h->castling;
  board->ep_row                                  = h->ep_row;
  board->ep_col                                  = h->ep_col;
  board->halfmove_clock                          = h->halfmove_clock;
  board->board[h->from_row][h->from_col]         = h->moving_piece;
  board->board[h->captured_row][h->captured_col] = h->captured_piece;
  board->selected_piece.active                   = false;
  board->drag.active                             = false;
  board->promo.active                            = false;

  if (cancelling_promotion)
    board->redo_count = 0;

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

  board_update_king_in_check(board);
  board->should_update_valid_moves = true;
  board_update_position_hash(board);
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
  board->drag.active                     = false;
  board->promo.active                    = false;

  bool irreversible = moving_piece == 'P' || moving_piece == 'p' || h->captured_piece != 0;
  board->halfmove_clock = next_halfmove_clock(h->halfmove_clock, irreversible);

  Move move = create_move(rowcol_to_sq(h->from_row, h->from_col), rowcol_to_sq(h->to_row, h->to_col));
  update_castling_rights(board, moving_piece, h->captured_piece, move);

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

  board_update_king_in_check(board);
  board->should_update_valid_moves = true;
  board_update_position_hash(board);
}

int get_halfmove_clock(const BoardState *board) {
  return board->halfmove_clock;
}

static bool threefold_repetition(const BoardState *board) {
  int reversible_plies = board->halfmove_clock;
  if (reversible_plies > board->undo_count)
    reversible_plies = board->undo_count;

  int repetitions = 0;
  for (int distance=0; distance <= reversible_plies; distance+=2) {
    int history_index = board->undo_count - distance;

    if (board->hash_history[history_index] == board->position_hash && ++repetitions >= 3)
      return true;
  }

  return false;
}

static void board_update_game_result(BoardState *board) {
  board->game_result = GAME_ONGOING;

  board->should_update_valid_moves = true;
  board_update_valid_moves(board);

  BitboardSet bbset = make_bitboards_from_charboard((const char (*)[8])board->board);

  if (board->valid_moves.count == 0) {
    if (king_in_check(&bbset, board->turn)) {
      const char *winner = board->turn == WHITE ? "Black" : "White";
      board->game_result           = GAME_CHECKMATE;
      board->selected_piece.active = false;
      board->drag.active           = false;
      ui_set_info("%s checkmates", winner);
    }
    else {
      end_game(board, GAME_STALEMATE, "Drawn by stalemate");
    }
    return;
  }

  if (threefold_repetition(board)) {
    end_game(board, DRAW_THREEFOLD_REPETITION, "Drawn by threefold repetition");
    return;
  }

  if (is_insufficient_material(&bbset)) {
    end_game(board, DRAW_INSUFFICIENT_MATERIAL, "Drawn by insufficient material");
    return;
  }

  if (board->halfmove_clock >= FIFTY_MOVE_RULE_PLY_LIMIT) {
    end_game(board, DRAW_FIFTY_MOVE, "Drawn by fifty-move rule");
    return;
  }
}

static void end_game(BoardState *board, GameResult result, const char *message) {
  board->game_result           = result;
  board->selected_piece.active = false;
  board->drag.active           = false;
  ui_set_info("%s", message);
}

static void adjust_castling_flags(uint8_t *c, char moved, int fr, int fc, char captured, int tr, int tc) {
  /* on KING */
  if (moved == 'K') *c &= ~(CASTLE_WK | CASTLE_WQ);
  if (moved == 'k') *c &= ~(CASTLE_BK | CASTLE_BQ);

  /* on ROOK */
  if (moved == 'R') {
    if (fr == 7 && fc == 0) *c &= ~CASTLE_WQ; // a1 rook
    if (fr == 7 && fc == 7) *c &= ~CASTLE_WK; // h1 rook
  }
  if (moved == 'r') {
    if (fr == 0 && fc == 0) *c &= ~CASTLE_BQ; // a8 rook
    if (fr == 0 && fc == 7) *c &= ~CASTLE_BK; // h8 rook
  }

  /* on captured home rook */
  if (captured == 'R') {
    if (tr == 7 && tc == 0) *c &= ~CASTLE_WQ;
    if (tr == 7 && tc == 7) *c &= ~CASTLE_WK;
  }
  if (captured == 'r') {
    if (tr == 0 && tc == 0) *c &= ~CASTLE_BQ;
    if (tr == 0 && tc == 7) *c &= ~CASTLE_BK;
  }
}

static void adjust_enpassant(int *ep_row, int *ep_col, char p, int fr, int fc, int tr) {
  bool is_pawn     = (p == 'P' || p == 'p');
  bool double_push = is_pawn && SDL_abs(fr - tr) == 2;

  if (double_push) {
    *ep_row = (fr + tr) / 2;
    *ep_col = fc;
  }

  else {
    *ep_row = NO_ENPASSANT;
    *ep_col = NO_ENPASSANT;
  }
}

static void adjust_castling_rook(BoardState *tmp_b, int king_to_col, int row) {
  bool kingside = (king_to_col == 6);

  if (kingside) {
    char rook = tmp_b->board[row][7];
    tmp_b->board[row][5] = rook;
    tmp_b->board[row][7] = 0;
  } else {
    char rook = tmp_b->board[row][0];
    tmp_b->board[row][3] = rook;
    tmp_b->board[row][0] = 0;
  }
}

static void adjust_promoting_pawn(BoardState *tmp_b, char promote, Turn T, int tr, int tc) {
  char p;

  if (T == WHITE)
    p = promote;
  else
    p = SDL_tolower(promote);

  tmp_b->board[tr][tc] = p;
  tmp_b->history[tmp_b->redo_count - 1].promoted_piece = p;
}
