/*

  * sockfish.h is the main header file for the Sockfish chess engine.
  * It defines the core data structures and types used throughout the engine. (primarily: SF_Context)
  * Therefore, it can also be considered as 'sf_types.h'
  * Any file in the project can include this header to utilize the benefits of the types here. (e.g Move)
  ! Because it has this kind of a mission, it does not have a corresponding .c file.

*/

#pragma once

#include "bitboard.h"
#include <stdbool.h>

typedef enum {
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8
} Square;

typedef enum {
  WHITE, BLACK
} Turn;

#define NO_ENPASSANT -1
#define CASTLE_NONE 0x00
#define CASTLE_WK   0x01
#define CASTLE_WQ   0x02
#define CASTLE_BK   0x04
#define CASTLE_BQ   0x08

/* == 16bit Move == */
typedef uint16_t Move;
typedef enum {
  MOVE_NORMAL     = 0,
  MOVE_PROMOTION  = 1 << 14,
  MOVE_EN_PASSANT = 2 << 14,
  MOVE_CASTLING   = 3 << 14,
} MoveType;
typedef enum {
  PROMOTE_QUEEN,
  PROMOTE_ROOK,
  PROMOTE_BISHOP,
  PROMOTE_KNIGHT,
} PromotionType;
// EXAMPLE: 00  -  00   - 000000 - 000000   |   0000011100001100 => E2=12 to E4=28
//         type   promo     to      from

typedef struct SF_Context {
  BitboardSet bitboard_set;
  Turn search_color;
  Move best;
  uint8_t castling_rights;
  Square enpassant_sq; // -1 for none
  bool *stop_requested;
} SF_Context;

/* ===== Move Utilities ===== */
#define create_move(from, to)             ((from) | ((to) << 6) | MOVE_NORMAL)
#define create_promotion(from, to, promo) ((from) | ((to) << 6) | ((promo) << 12) | MOVE_PROMOTION)
#define create_en_passant(from, to)       ((from) | ((to) << 6) | MOVE_EN_PASSANT)
#define create_castling(from, to)         ((from) | ((to) << 6) | MOVE_CASTLING)

#define move_from(move)      ((move) & 0x3F)         // bits 0-5
#define move_to(move)        (((move) >> 6) & 0x3F)  // bits 6-11
#define move_promotion(move) (((move) >> 12) & 0x3)  // bits 12-13
#define move_type(move)      ((move) & 0xC000)       // bits 14-15

#define square_to_row(sq)     (sq / 8)
#define square_to_col(sq)     (sq % 8)
#define rowcol_to_sq(row,col) (row * 8 + col)

static inline void sq_to_alg(Square sq, char buf[3]) {
  /* square (from-to) -> algebraic (e2-e4) */
  int row = square_to_row(sq);
  int col = square_to_col(sq);
  buf[0] = 'a' + col;
  buf[1] = '1' + row;
  buf[2] = '\0';
}

/* ===== Sockish Functions & Algorithm =====

Move     sf_search(const SF_Context *ctx);          // search.c
MoveList sf_generate_moves(const SF_Context *ctx);  // movegen.c

*/