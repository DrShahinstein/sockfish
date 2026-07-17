#pragma once

#include "sockfish.h"

typedef enum {
  TT_EXACT,
  TT_ALPHA,
  TT_BETA,
} TT_Flag;

typedef struct {
  U64 hash_key;
  int score;
  Move best_move;
  uint8_t depth;
  uint8_t flag;
} TT_Entry;

extern TT_Entry *tt_table;
extern int tt_num_entries;

void tt_init(int size_mb);
void tt_clear(void);
void tt_free(void);

bool tt_probe(U64 hash_key, int depth, int alpha, int beta, int *return_score, Move *best_move);
void tt_record(U64 hash_key, int depth, int score, TT_Flag flag, Move best_move);
int tt_get_hashfull(void);

