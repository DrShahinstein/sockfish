#include "sockfish/move_helper.h"
#include "sockfish/movegen.h"
#include "sockfish/evaluation.h" // for PeSTO tables and etc.
#include <stdlib.h>
#include <string.h>

/* Internal Helpers */
static inline void remove_piece(BitboardSet *bbs, Square sq, PieceType piece);
static inline void place_piece(BitboardSet *bbs, Square sq, PieceType piece);
static inline PieceType get_promotion_piece(Move move, Turn color);

/* Procedurally Refactored Funcs */
static void update_incremental_eval(SF_Context *ctx, Move move, PieceType moving_piece, PieceType captured_piece, Square captured_square);
static void move_pieces_on_board(SF_Context *ctx, Move move, PieceType moving_piece, PieceType captured_piece, Square captured_square);
static void update_castling_rights(SF_Context *ctx, Square from, PieceType moving_piece, PieceType captured_piece, Square captured_square);
static void update_en_passant(SF_Context *ctx, Move move, PieceType moving_piece);

void make_move(SF_Context *ctx, Move move, MoveHistory *history) {
  history->move                 = move;
  history->prev_castling        = ctx->castling_rights;
  history->prev_ep_sq           = ctx->enpassant_sq;
  history->prev_hash            = ctx->hash_key;
  history->prev_mg_score[WHITE] = ctx->mg_score[WHITE];
  history->prev_mg_score[BLACK] = ctx->mg_score[BLACK];
  history->prev_eg_score[WHITE] = ctx->eg_score[WHITE];
  history->prev_eg_score[BLACK] = ctx->eg_score[BLACK];
  history->prev_game_phase      = ctx->game_phase;

  Square from            = move_from(move);
  Square to              = move_to(move);
  MoveType type          = move_type(move);
  Turn us                = ctx->search_color;
  PieceType moving_piece = get_piece_type(&ctx->bitboard_set, from);

  history->captured_square = to;
  if (type == MOVE_EN_PASSANT) {
    int ep_offset = (us == WHITE) ? -8 : 8;
    history->captured_square = to + ep_offset;
  }
  history->captured_piece = get_piece_type(&ctx->bitboard_set, history->captured_square);

  update_incremental_eval(ctx, move, moving_piece, history->captured_piece, history->captured_square);
  move_pieces_on_board(ctx, move, moving_piece, history->captured_piece, history->captured_square);
  update_castling_rights(ctx, from, moving_piece, history->captured_piece, history->captured_square);
  update_en_passant(ctx, move, moving_piece);

  ctx->hash_key                          ^= zobrist_black_to_move;
  ctx->search_color                       = !ctx->search_color;
  ctx->pos_history[ctx->history_count++]  = ctx->hash_key;
}

void unmake_move(SF_Context *ctx, const MoveHistory *history) {
  ctx->hash_key         = history->prev_hash;
  ctx->history_count   -= 1;
  ctx->mg_score[WHITE]  = history->prev_mg_score[WHITE];
  ctx->mg_score[BLACK]  = history->prev_mg_score[BLACK];
  ctx->eg_score[WHITE]  = history->prev_eg_score[WHITE];
  ctx->eg_score[BLACK]  = history->prev_eg_score[BLACK];
  ctx->game_phase       = history->prev_game_phase;
  ctx->castling_rights  = history->prev_castling;
  ctx->enpassant_sq     = history->prev_ep_sq;
  ctx->search_color     = !ctx->search_color;

  Move move              = history->move;
  Square from            = move_from(move);
  Square to              = move_to(move);
  MoveType type          = move_type(move);
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
    Square rook_from=-1, rook_to=-1;

    switch (to) {
      case G1: rook_from=H1; rook_to=F1; break;
      case C1: rook_from=A1; rook_to=D1; break;
      case G8: rook_from=H8; rook_to=F8; break;
      case C8: rook_from=A8; rook_to=D8; break;
      default:                           break;
    }

    PieceType rook = (ctx->search_color == WHITE) ? W_ROOK : B_ROOK;
    remove_piece(&ctx->bitboard_set, rook_to, rook);
    place_piece (&ctx->bitboard_set, rook_from, rook);
  }
}

bool king_in_check(const BitboardSet *bbset, Turn color) {
  Square king_sq = GET_LSB(bbset->kings[color]);
  Turn opponent  = !color;
  return square_attacked(bbset, king_sq, opponent);
}

void make_null_move(SF_Context *ctx, MoveHistory *history) {
  history->prev_hash            = ctx->hash_key;
  history->prev_ep_sq           = ctx->enpassant_sq;
  history->prev_mg_score[WHITE] = ctx->mg_score[WHITE];
  history->prev_mg_score[BLACK] = ctx->mg_score[BLACK];
  history->prev_eg_score[WHITE] = ctx->eg_score[WHITE];
  history->prev_eg_score[BLACK] = ctx->eg_score[BLACK];
  history->prev_game_phase      = ctx->game_phase;

  if ((int)ctx->enpassant_sq != NO_ENPASSANT) {
    ctx->hash_key     ^= zobrist_enpassant[ctx->enpassant_sq % 8];
    ctx->enpassant_sq  = NO_ENPASSANT;
  }

  ctx->hash_key                         ^= zobrist_black_to_move;
  ctx->search_color                      = !ctx->search_color;
  ctx->pos_history[ctx->history_count++] = ctx->hash_key;
}

void unmake_null_move(SF_Context *ctx, const MoveHistory *history) {
  ctx->hash_key         = history->prev_hash;
  ctx->enpassant_sq     = history->prev_ep_sq;
  ctx->search_color     = !ctx->search_color;
  ctx->history_count   -= 1;
  ctx->mg_score[WHITE]  = history->prev_mg_score[WHITE];
  ctx->mg_score[BLACK]  = history->prev_mg_score[BLACK];
  ctx->eg_score[WHITE]  = history->prev_eg_score[WHITE];
  ctx->eg_score[BLACK]  = history->prev_eg_score[BLACK];
  ctx->game_phase       = history->prev_game_phase;
}

PieceType get_piece_type(const BitboardSet *bbs, Square sq) {
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

/* --- */

static void update_incremental_eval(SF_Context *ctx, Move move, PieceType moving_piece, PieceType captured_piece, Square captured_square) {
  Turn us           = ctx->search_color;
  Turn them         = !us;
  Square from       = move_from(move);
  Square to         = move_to(move);
  MoveType type     = move_type(move);
  PestoIdx p_idx    = moving_piece % 6; 
  int from_table_sq = (us == WHITE) ? flip(from) : from;
  int to_table_sq   = (us == WHITE) ? flip(to)   : to;

  ctx->mg_score[us] -= (PESTO_TABLE[p_idx][MG][from_table_sq] + MG_MATERIAL[p_idx]);
  ctx->eg_score[us] -= (PESTO_TABLE[p_idx][EG][from_table_sq] + EG_MATERIAL[p_idx]);

  if (type == MOVE_PROMOTION) {
    PieceType promo    = get_promotion_piece(move, us);
    PestoIdx promo_idx = promo % 6;
    
    ctx->mg_score[us] += (PESTO_TABLE[promo_idx][MG][to_table_sq] + MG_MATERIAL[promo_idx]);
    ctx->eg_score[us] += (PESTO_TABLE[promo_idx][EG][to_table_sq] + EG_MATERIAL[promo_idx]);
    
    int phase_values[6] = {PHASE_PAWN, PHASE_KNIGHT, PHASE_BISHOP, PHASE_ROOK, PHASE_QUEEN, 0};
    ctx->game_phase += phase_values[promo_idx] - PHASE_PAWN; 
  } else {
    ctx->mg_score[us] += (PESTO_TABLE[p_idx][MG][to_table_sq] + MG_MATERIAL[p_idx]);
    ctx->eg_score[us] += (PESTO_TABLE[p_idx][EG][to_table_sq] + EG_MATERIAL[p_idx]);
  }

  if (captured_piece != NO_PIECE) {
    PestoIdx cap_idx = captured_piece % 6;
    int cap_table_sq = (them == WHITE) ? flip(captured_square) : captured_square;
    
    ctx->mg_score[them] -= (PESTO_TABLE[cap_idx][MG][cap_table_sq] + MG_MATERIAL[cap_idx]);
    ctx->eg_score[them] -= (PESTO_TABLE[cap_idx][EG][cap_table_sq] + EG_MATERIAL[cap_idx]);
    
    int phase_values[6] = { PHASE_PAWN, PHASE_KNIGHT, PHASE_BISHOP, PHASE_ROOK, PHASE_QUEEN, 0 };
    ctx->game_phase    -= phase_values[cap_idx];
  }

  if (type == MOVE_CASTLING) {
    Square rook_from=-1, rook_to=-1;

    switch (to) {
      case G1: rook_from=H1; rook_to=F1; break;
      case C1: rook_from=A1; rook_to=D1; break;
      case G8: rook_from=H8; rook_to=F8; break;
      case C8: rook_from=A8; rook_to=D8; break;
      default:                           break;
    }
    
    int r_from_table_sq = (us == WHITE) ? flip(rook_from) : rook_from;
    int r_to_table_sq   = (us == WHITE) ? flip(rook_to)   : rook_to;
    
    ctx->mg_score[us] -= PESTO_TABLE[PST_ROOK][MG][r_from_table_sq];
    ctx->eg_score[us] -= PESTO_TABLE[PST_ROOK][EG][r_from_table_sq];
    ctx->mg_score[us] += PESTO_TABLE[PST_ROOK][MG][r_to_table_sq];
    ctx->eg_score[us] += PESTO_TABLE[PST_ROOK][EG][r_to_table_sq];
  }
}

static void move_pieces_on_board(SF_Context *ctx, Move move, PieceType moving_piece, PieceType captured_piece, Square captured_square) {
  Square from   = move_from(move);
  Square to     = move_to(move);
  MoveType type = move_type(move);
  Turn us       = ctx->search_color;

  ctx->hash_key ^= zobrist_pieces[moving_piece][from];
  remove_piece(&ctx->bitboard_set, from, moving_piece);

  if (captured_piece != NO_PIECE) {
    ctx->hash_key ^= zobrist_pieces[captured_piece][captured_square];
    remove_piece(&ctx->bitboard_set, captured_square, captured_piece);
  }

  if (type == MOVE_PROMOTION) {
    PieceType promoted_piece = get_promotion_piece(move, us);
    ctx->hash_key ^= zobrist_pieces[promoted_piece][to];
    place_piece(&ctx->bitboard_set, to, promoted_piece);
  } else {
    ctx->hash_key ^= zobrist_pieces[moving_piece][to];
    place_piece(&ctx->bitboard_set, to, moving_piece);
  }

  if (type == MOVE_CASTLING) {
    Square rook_from=-1, rook_to=-1;

    switch (to) {
      case G1: rook_from=H1; rook_to=F1; break;
      case C1: rook_from=A1; rook_to=D1; break;
      case G8: rook_from=H8; rook_to=F8; break;
      case C8: rook_from=A8; rook_to=D8; break;
      default:                           break;
    }

    PieceType rook = (us == WHITE) ? W_ROOK : B_ROOK;
    
    ctx->hash_key ^= zobrist_pieces[rook][rook_from];
    ctx->hash_key ^= zobrist_pieces[rook][rook_to];

    remove_piece(&ctx->bitboard_set, rook_from, rook);
    place_piece (&ctx->bitboard_set, rook_to,   rook);
  }
}

static void update_castling_rights(SF_Context *ctx, Square from, PieceType moving_piece, PieceType captured_piece, Square captured_square) {
  uint8_t old_rights = ctx->castling_rights;

  if      (moving_piece == W_KING) ctx->castling_rights &= ~(CASTLE_WK | CASTLE_WQ);
  else if (moving_piece == B_KING) ctx->castling_rights &= ~(CASTLE_BK | CASTLE_BQ);
  else if (moving_piece == W_ROOK) {
    if (from == H1) ctx->castling_rights &= ~CASTLE_WK;
    if (from == A1) ctx->castling_rights &= ~CASTLE_WQ;
  } else if (moving_piece == B_ROOK) {
    if (from == H8) ctx->castling_rights &= ~CASTLE_BK;
    if (from == A8) ctx->castling_rights &= ~CASTLE_BQ;
  }

  if (captured_piece == W_ROOK) {
    if      (captured_square == H1) ctx->castling_rights &= ~CASTLE_WK;
    else if (captured_square == A1) ctx->castling_rights &= ~CASTLE_WQ;
  }
  else if (captured_piece == B_ROOK) {
    if      (captured_square == H8) ctx->castling_rights &= ~CASTLE_BK;
    else if (captured_square == A8) ctx->castling_rights &= ~CASTLE_BQ;
  }

  ctx->hash_key ^= zobrist_castling[old_rights];
  ctx->hash_key ^= zobrist_castling[ctx->castling_rights];
}

static void update_en_passant(SF_Context *ctx, Move move, PieceType moving_piece) {
  Square from   = move_from(move);
  Square to     = move_to(move);
  MoveType type = move_type(move);
  Square old_ep = ctx->enpassant_sq;

  if (type == MOVE_NORMAL && (moving_piece == W_PAWN || moving_piece == B_PAWN)) {
    int from_rank = from / 8;
    int to_rank   = to   / 8;

    bool double_pawn_push = abs(from_rank - to_rank) == 2;
    if (double_pawn_push) {
      ctx->enpassant_sq = (from + to) / 2;
    } else ctx->enpassant_sq = NO_ENPASSANT;
  } 
  else {
    ctx->enpassant_sq = NO_ENPASSANT;
  }

  if ((int) old_ep != NO_ENPASSANT) {
    ctx->hash_key ^= zobrist_enpassant[old_ep % 8];
  }
  if ((int) ctx->enpassant_sq != NO_ENPASSANT) {
    ctx->hash_key ^= zobrist_enpassant[ctx->enpassant_sq % 8];
  }
}

/* --- */

static inline void remove_piece(BitboardSet *bbs, Square sq, PieceType piece) {
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

static inline void place_piece(BitboardSet *bbs, Square sq, PieceType piece) {
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

static inline PieceType get_promotion_piece(Move move, Turn color) {
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

