#include "sockfish.h"
#include "move_helper.h"  // has_legal_en_passant_capture()

U64 zobrist_pieces[12][64];
U64 zobrist_black_to_move;
U64 zobrist_castling[16];
U64 zobrist_enpassant[8];
U64 zobrist_rule50[FIFTY_MOVE_RULE_PLY_LIMIT];
U64 zobrist_null_search;

static U64 rand64(void) {
  static U64 seed = 0x98f107b4e56feULL;
  seed ^= seed >> 12;
  seed ^= seed << 25;
  seed ^= seed >> 27;
  return seed * 0x2545F4914F6CDD1DULL;
}

void init_zobrist_keys(void) {
  for (int piece=0; piece < 12; ++piece) {
    for (int square=0; square < 64; ++square) {
      zobrist_pieces[piece][square] = rand64();
    }
  }

  zobrist_black_to_move = rand64();
  
  for (int i=0; i < 16; ++i) {
    zobrist_castling[i] = rand64();
  }
  
  for (int i=0; i < 8; ++i) {
    zobrist_enpassant[i] = rand64();
  }

  for (int i=0; i < FIFTY_MOVE_RULE_PLY_LIMIT; ++i) {
    zobrist_rule50[i] = rand64();
  }

  zobrist_null_search = rand64();
}

void sf_init_hash_key(SF_Context *ctx) {
  U64 hash = 0;

  ctx->castling_rights &= CASTLE_ALL;

  if (!GET_BIT(ctx->bitboard_set.kings[WHITE], E1))
    ctx->castling_rights &= ~(CASTLE_WK | CASTLE_WQ);
  if (!GET_BIT(ctx->bitboard_set.rooks[WHITE], H1))
    ctx->castling_rights &= ~CASTLE_WK;
  if (!GET_BIT(ctx->bitboard_set.rooks[WHITE], A1))
    ctx->castling_rights &= ~CASTLE_WQ;

  if (!GET_BIT(ctx->bitboard_set.kings[BLACK], E8))
    ctx->castling_rights &= ~(CASTLE_BK | CASTLE_BQ);
  if (!GET_BIT(ctx->bitboard_set.rooks[BLACK], H8))
    ctx->castling_rights &= ~CASTLE_BK;
  if (!GET_BIT(ctx->bitboard_set.rooks[BLACK], A8))
    ctx->castling_rights &= ~CASTLE_BQ;

  if ((int)ctx->enpassant_sq != NO_ENPASSANT && !has_legal_en_passant_capture(ctx)) {
    ctx->enpassant_sq = NO_ENPASSANT;
  }
  
  for (int color = WHITE; color <= BLACK; ++color) {
    for (int pt = 0; pt < 6; ++pt) {
      PieceType piece = (color == WHITE) ? pt : pt + 6;

      U64 bitboard=0;
      if      (pt == 0) bitboard = ctx->bitboard_set.pawns[color];
      else if (pt == 1) bitboard = ctx->bitboard_set.knights[color];
      else if (pt == 2) bitboard = ctx->bitboard_set.bishops[color];
      else if (pt == 3) bitboard = ctx->bitboard_set.rooks[color];
      else if (pt == 4) bitboard = ctx->bitboard_set.queens[color];
      else if (pt == 5) bitboard = ctx->bitboard_set.kings[color];

      while (bitboard) {
        Square sq = POP_LSB(&bitboard);
        hash ^= zobrist_pieces[piece][sq];
      }
    }
  }

  if (ctx->search_color == BLACK) {
    hash ^= zobrist_black_to_move;
  }

  hash ^= zobrist_castling[ctx->castling_rights];
  
  if ((int) ctx->enpassant_sq != NO_ENPASSANT ) {
    hash ^= zobrist_enpassant[square_to_col(ctx->enpassant_sq)];
  }

  ctx->hash_key = hash;
}

U64 sf_tt_hash_key(const SF_Context *ctx) {
  int rule50 = ctx->halfmove_clock;
  if (rule50 < 0)  rule50 = 0;
  if (rule50 >= FIFTY_MOVE_RULE_PLY_LIMIT)
    rule50 = FIFTY_MOVE_RULE_PLY_LIMIT - 1;

  U64 key = ctx->hash_key ^ zobrist_rule50[rule50];
  if (ctx->in_null_search)
    key ^= zobrist_null_search;

  return key;
}
