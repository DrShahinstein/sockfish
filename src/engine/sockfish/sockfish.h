/*

  * sockfish.h is the main header file for the Sockfish chess engine.
  * It defines the core data structures and types used throughout the engine. (primarily: SF_Context)
  * Therefore, it can also be considered as 'sf_types.h'
  * Any file in the project can include this header to utilize the benefits of the types here. (e.g MoveRC, MoveSQ)
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

// MoveRC => move with row-col coordinates (board/ui friendly)
typedef struct {
  int fr; int fc; // (0-7),(0,7)
  int tr; int tc; // (0-7),(0,7)
} MoveRC;

// MoveSQ => move with bit coordinates (engine friendly)
typedef struct {
  Square from; // 0-63
  Square to;   // 0-63
} MoveSQ;

typedef struct SF_Context {
  BitboardSet bitboard_set;
  Turn search_color;
  MoveSQ best;
  bool thinking;
} SF_Context;

/* ===== (sq - rowcol) convertions ===== */
static inline void sq_to_rowcol(Square sq, int *row, int *col) {
  *row = sq / 8;
  *col = sq % 8;
}
static inline int rowcol_to_sq(int row, int col) {
  return row * 8 + col;
}
static inline void sq_to_alg(Square sq, char buf[3]) {
  int row, col;
  sq_to_rowcol(sq, &row, &col);
  buf[0] = 'a' + col;
  buf[1] = '1' + row;
  buf[2] = '\0';
} //

/* ===== Sockish Functions & Algorithm =====

Move sf_search(const SF_Context *ctx);                             // search.c
MoveList sf_generate_moves(const BitboardSet *bbset, Turn color);  // movegen.c

*/