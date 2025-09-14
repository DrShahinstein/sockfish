#include "sockfish/move_helper.h"
#include "sockfish/movegen.h"
#include <stdlib.h>
#include <string.h>

static PieceType get_piece_type(const BitboardSet *bbs, Square sq);
static void      remove_piece(BitboardSet *bbs, Square sq, PieceType piece);
static void      place_piece(BitboardSet *bbs, Square sq, PieceType piece);
static PieceType get_promotion_piece(Move move, Turn color);

void make_move(SF_Context *ctx, Move move, MoveHistory *history) {
  history->move          = move;
  history->prev_castling = ctx->castling_rights;
  history->prev_ep_sq    = ctx->enpassant_sq;

  Square from   = move_from(move);
  Square to     = move_to(move);
  MoveType type = move_type(move);

  PieceType moving_piece = get_piece_type(&ctx->bitboard_set, from);

  history->captured_piece  = NO_PIECE;
  history->captured_square = to;

  if (type == MOVE_EN_PASSANT) {
    int ep_offset            = (ctx->search_color == WHITE) ? -8 : 8;
    history->captured_square = to + ep_offset;
    history->captured_piece  = get_piece_type(&ctx->bitboard_set, history->captured_square);
  } else {
    history->captured_piece  = get_piece_type(&ctx->bitboard_set, to);
  }

  remove_piece(&ctx->bitboard_set, from, moving_piece);

  if (history->captured_piece != NO_PIECE) {
    remove_piece(&ctx->bitboard_set, history->captured_square, history->captured_piece);
  }

  if (type == MOVE_PROMOTION) {
    PieceType promoted_piece = get_promotion_piece(move, ctx->search_color);
    place_piece(&ctx->bitboard_set, to, promoted_piece);
  } else place_piece(&ctx->bitboard_set, to, moving_piece);

  if (history->captured_piece != NO_PIECE) {
    place_piece(&ctx->bitboard_set, to, moving_piece);
  }

  if (type == MOVE_CASTLING) {
    Square rook_from, rook_to;
    if (to == G1) {        // white-kingside
      rook_from = H1;
      rook_to   = F1;
    } else if (to == C1) { // white-queenside
      rook_from = A1;
      rook_to   = D1;
    } else if (to == G8) { // black-kingside
      rook_from = H8;
      rook_to   = F8;
    } else if (to == C8) { // black-queenside
      rook_from = A8;
      rook_to   = D8;
    }

    PieceType rook = (ctx->search_color == WHITE) ? W_ROOK : B_ROOK;
    remove_piece(&ctx->bitboard_set, rook_from, rook);
    place_piece (&ctx->bitboard_set, rook_to,   rook);
  }

  if (moving_piece == W_KING) ctx->castling_rights &= ~(CASTLE_WK | CASTLE_WQ);
  else if (moving_piece == B_KING) ctx->castling_rights &= ~(CASTLE_BK | CASTLE_BQ);
  else if (moving_piece == W_ROOK) {
    if (from == H1) ctx->castling_rights &= ~CASTLE_WK;
    if (from == A1) ctx->castling_rights &= ~CASTLE_WQ;
  } else if (moving_piece == B_ROOK) {
    if (from == H8) ctx->castling_rights &= ~CASTLE_BK;
    if (from == A8) ctx->castling_rights &= ~CASTLE_BQ;
  }

  if (type == MOVE_NORMAL && (moving_piece == W_PAWN || moving_piece == B_PAWN)) {
    int from_rank = from / 8;
    int to_rank   = to   / 8;

    if (abs(from_rank - to_rank) == 2) { // double pawn push
      ctx->enpassant_sq      = (from + to) / 2;
    } else ctx->enpassant_sq = NO_ENPASSANT;
  } else {
    ctx->enpassant_sq = NO_ENPASSANT;
  }

  ctx->search_color = !ctx->search_color;
}

void unmake_move(SF_Context *ctx, const MoveHistory *history) {
  Move move     = history->move;
  Square from   = move_from(move);
  Square to     = move_to(move);
  MoveType type = move_type(move);

  ctx->search_color = !ctx->search_color;

  PieceType moving_piece = get_piece_type(&ctx->bitboard_set, to);

  if (type == MOVE_PROMOTION) {
    remove_piece(&ctx->bitboard_set, to, moving_piece);
    moving_piece = (ctx->search_color == WHITE) ? W_PAWN : B_PAWN;
    place_piece(&ctx->bitboard_set, to, moving_piece);
  }

  remove_piece(&ctx->bitboard_set, to,   moving_piece);
  place_piece (&ctx->bitboard_set, from, moving_piece);

  if (history->captured_piece != NO_PIECE) {
    place_piece(&ctx->bitboard_set, history->captured_square, history->captured_piece);
  }

  if (type == MOVE_CASTLING) {
    Square rook_from, rook_to;

    if (to == G1) {        // white-kingside
      rook_from = H1;
      rook_to   = F1;
    } else if (to == C1) { // white-queenside
      rook_from = A1;
      rook_to   = D1;
    } else if (to == G8) { // black-kingside
      rook_from = H8;
      rook_to   = F8;
    } else if (to == C8) { // black-queenside
      rook_from = A8;
      rook_to   = D8;
    }

    PieceType rook = (ctx->search_color == WHITE) ? W_ROOK : B_ROOK;
    remove_piece(&ctx->bitboard_set, rook_to, rook);
    place_piece (&ctx->bitboard_set, rook_from, rook);
  }

  ctx->castling_rights = history->prev_castling;
  ctx->enpassant_sq    = history->prev_ep_sq;
}

bool king_in_check(const SF_Context *ctx, Turn color) {
  Square king_sq = GET_LSB(ctx->bitboard_set.kings[color]);
  Turn opponent  = !color;
  U64 attacks    = compute_attacks(&ctx->bitboard_set, opponent);
  return (attacks & (1ULL << king_sq)) != 0;
}

static PieceType get_piece_type(const BitboardSet *bbs, Square sq) {
  if (GET_BIT(bbs->pawns[WHITE], sq))   return W_PAWN;
  if (GET_BIT(bbs->pawns[BLACK], sq))   return B_PAWN;
  if (GET_BIT(bbs->knights[WHITE], sq)) return W_KNIGHT;
  if (GET_BIT(bbs->knights[BLACK], sq)) return B_KNIGHT;
  if (GET_BIT(bbs->bishops[WHITE], sq)) return W_BISHOP;
  if (GET_BIT(bbs->bishops[BLACK], sq)) return B_BISHOP;
  if (GET_BIT(bbs->rooks[WHITE], sq))   return W_ROOK;
  if (GET_BIT(bbs->rooks[BLACK], sq))   return B_ROOK;
  if (GET_BIT(bbs->queens[WHITE], sq))  return W_QUEEN;
  if (GET_BIT(bbs->queens[BLACK], sq))  return B_QUEEN;
  if (GET_BIT(bbs->kings[WHITE], sq))   return W_KING;
  if (GET_BIT(bbs->kings[BLACK], sq))   return B_KING;
  return NO_PIECE;
}

static void remove_piece(BitboardSet *bbs, Square sq, PieceType piece) {
  switch (piece) {
  case W_PAWN:   CLEAR_BIT(bbs->pawns[WHITE], sq);   break;
  case B_PAWN:   CLEAR_BIT(bbs->pawns[BLACK], sq);   break;
  case W_KNIGHT: CLEAR_BIT(bbs->knights[WHITE], sq); break;
  case B_KNIGHT: CLEAR_BIT(bbs->knights[BLACK], sq); break;
  case W_BISHOP: CLEAR_BIT(bbs->bishops[WHITE], sq); break;
  case B_BISHOP: CLEAR_BIT(bbs->bishops[BLACK], sq); break;
  case W_ROOK:   CLEAR_BIT(bbs->rooks[WHITE], sq);   break;
  case B_ROOK:   CLEAR_BIT(bbs->rooks[BLACK], sq);   break;
  case W_QUEEN:  CLEAR_BIT(bbs->queens[WHITE], sq);  break;
  case B_QUEEN:  CLEAR_BIT(bbs->queens[BLACK], sq);  break;
  case W_KING:   CLEAR_BIT(bbs->kings[WHITE], sq);   break;
  case B_KING:   CLEAR_BIT(bbs->kings[BLACK], sq);   break;
  default:                                           break;
  }

  CLEAR_BIT(bbs->all_pieces[WHITE], sq);
  CLEAR_BIT(bbs->all_pieces[BLACK], sq);
  CLEAR_BIT(bbs->occupied, sq);
}

static void place_piece(BitboardSet *bbs, Square sq, PieceType piece) {
  switch (piece) {
  case W_PAWN:   SET_BIT(bbs->pawns[WHITE], sq);   break;
  case B_PAWN:   SET_BIT(bbs->pawns[BLACK], sq);   break;
  case W_KNIGHT: SET_BIT(bbs->knights[WHITE], sq); break;
  case B_KNIGHT: SET_BIT(bbs->knights[BLACK], sq); break;
  case W_BISHOP: SET_BIT(bbs->bishops[WHITE], sq); break;
  case B_BISHOP: SET_BIT(bbs->bishops[BLACK], sq); break;
  case W_ROOK:   SET_BIT(bbs->rooks[WHITE], sq);   break;
  case B_ROOK:   SET_BIT(bbs->rooks[BLACK], sq);   break;
  case W_QUEEN:  SET_BIT(bbs->queens[WHITE], sq);  break;
  case B_QUEEN:  SET_BIT(bbs->queens[BLACK], sq);  break;
  case W_KING:   SET_BIT(bbs->kings[WHITE], sq);   break;
  case B_KING:   SET_BIT(bbs->kings[BLACK], sq);   break;
  default:                                         break;
  }

  Turn color = (piece <= W_KING) ? WHITE : BLACK;
  SET_BIT(bbs->all_pieces[color], sq);
  SET_BIT(bbs->occupied, sq);
}

static PieceType get_promotion_piece(Move move, Turn color) {
  PromotionType promo = move_promotion(move);
  if (color == WHITE) {
    switch (promo) {
    case PROMOTE_QUEEN:  return W_QUEEN;
    case PROMOTE_ROOK:   return W_ROOK;
    case PROMOTE_BISHOP: return W_BISHOP;
    case PROMOTE_KNIGHT: return W_KNIGHT;
    }
  } else {
    switch (promo) {
    case PROMOTE_QUEEN:  return B_QUEEN;
    case PROMOTE_ROOK:   return B_ROOK;
    case PROMOTE_BISHOP: return B_BISHOP;
    case PROMOTE_KNIGHT: return B_KNIGHT;
    }
  }
  return NO_PIECE; // should not happen
}