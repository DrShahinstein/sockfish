#pragma once

#include "sockfish.h"
#include <stdatomic.h>

/*
 * 64-bit TT Payload Bit Layout:
 *
 *  [62]       [57..56]    [55..48]    [47..32]    [31..0]
 *   |            |           |           |           |
 *   |            |           |           |           +-- Score (32 bit, int32_t)
 *   |            |           |           +-------------- Best Move (16 bit, Move)
 *   |            |           +-------------------------- Depth (8 bit, uint8_t)
 *   |            +-------------------------------------- Flag (2 bit, TT_Flag)
 *   +--------------------------------------------------- Occupied Flag (1 bit)
 */
#define TT_SCORE_SHIFT     0
#define TT_MOVE_SHIFT     32
#define TT_DEPTH_SHIFT    48
#define TT_FLAG_SHIFT     56
#define TT_OCCUPIED_MASK  (UINT64_C(1) << 62)

typedef enum {
  TT_EXACT,
  TT_ALPHA,
  TT_BETA,
} TT_Flag;

typedef struct {
  _Atomic U64 signature;
  _Atomic U64 payload;
} TT_Entry;
_Static_assert(sizeof(TT_Entry) == 16, "TT_Entry must be 16 bytes");

typedef struct {
  int32_t score;
  Move best_move;
  uint8_t depth;
  TT_Flag flag;
} TT_Data;

extern TT_Entry *tt_table;
extern int tt_num_entries;

void tt_init(int size_mb);
void tt_clear(void);
void tt_free(void);

bool tt_probe(U64 hash_key, TT_Data *data);
void tt_record(U64 hash_key, int depth, int score, TT_Flag flag, Move best_move);
int tt_get_hashfull(void);

