#pragma once

#include "bitboard.h"
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
  W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
  B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
  NO_PIECE=-1
} PieceType;

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
#define CASTLE_ALL  0x0F

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

#define SF_MAX_HIST 1024
#define SF_MAX_PLY 128

typedef struct SF_Context {
  BitboardSet bitboard_set;
  U64 nodes;
  U64 start_time;
  U64 time_limit;
  U64 hash_key;
  U64 pos_history[SF_MAX_HIST];
  Move killer_moves[SF_MAX_PLY][2];
  int mg_score[2];                     // [0]=WHITE, [1]=BLACK [mid-game score]
  int eg_score[2];                     // [0]=WHITE, [1]=BLACK [end-game score]
  int game_phase;                      // 0-24                 [see evaluation.h]
  int history_count;
  int halfmove_clock;                  // helps tracking 50-move-draw
  bool *should_stop;
  Turn search_color;
  Square enpassant_sq;
  Move best;
  uint8_t castling_rights;
} SF_Context;

SF_Context create_sf_ctx(BitboardSet *bitboard_set, Turn search_color, uint8_t castling_rights, Square ep_sq); // sockfish.c

/* ==== ZOBRIST (zobrist.c) ==== */
extern U64 zobrist_pieces[12][64];
extern U64 zobrist_black_to_move;
extern U64 zobrist_castling[16];
extern U64 zobrist_enpassant[8];

void init_zobrist_keys(void);
void sf_init_hash_key(SF_Context *ctx);
/* -end- */


/* ===== Move Utilities ===== */
#define create_move(from, to)             ((from) | ((to) << 6) | MOVE_NORMAL)
#define create_promotion(from, to, promo) ((from) | ((to) << 6) | ((promo) << 12) | MOVE_PROMOTION)
#define create_en_passant(from, to)       ((from) | ((to) << 6) | MOVE_EN_PASSANT)
#define create_castling(from, to)         ((from) | ((to) << 6) | MOVE_CASTLING)

#define move_from(move)      ((move) & 0x3F)         // bits 0-5
#define move_to(move)        (((move) >> 6) & 0x3F)  // bits 6-11
#define move_promotion(move) (((move) >> 12) & 0x3)  // bits 12-13
#define move_type(move)      ((move) & 0xC000)       // bits 14-15

#define square_to_row(sq)     (7 - sq / 8)
#define square_to_col(sq)     (sq % 8)
#define rowcol_to_sq(row,col) ((7-row) * 8 + col)

/* ==== Other Helpers ==== */
static inline void sq_to_alg(Square sq, char buf[3]) {
  int row = sq / 8;
  int col = sq % 8;
  buf[0]  = 'a' + col;
  buf[1]  = '1' + row;
  buf[2]  = '\0';
}

static inline bool should_stop(const SF_Context *ctx) {
  return ctx->should_stop && *ctx->should_stop;
}


/* ===== Sockish Functions =====
 *
 * Move     sf_search(const SF_Context *ctx);            // search.c
 * MoveList sf_generate_moves(const SF_Context *ctx);    // movegen.c
 * int      sf_evaluate_position(const SF_Context *ctx); // evaluation.c
 * void     sf_init_evaluation(SF_Context *ctx);         // evaluation.c
 * void     sf_init_hash_key(SF_Context *ctx);           // zobrist.c
 *
 */
