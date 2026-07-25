#include "transposition_table.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static U64 tt_pack_payload(int32_t score, Move best_move, uint8_t depth, TT_Flag flag);
static int32_t tt_unpack_score(U64 payload);
static Move tt_unpack_move(U64 payload);
static uint8_t tt_unpack_depth(U64 payload);
static TT_Flag tt_unpack_flag(U64 payload);

TT_Entry *tt_table = NULL; // this is the transposition table
int tt_num_entries = 0;

void tt_init(int size_mb) {
  if (tt_table != NULL) {
    tt_free();
  }

  if (size_mb <= 0) {
    tt_num_entries = 0;
    tt_table = NULL;
    return;
  }

  size_t bytes   = (size_t)size_mb * 1024 * 1024;
  tt_num_entries = bytes / sizeof(TT_Entry);

  tt_table = (TT_Entry *)malloc((size_t)tt_num_entries * sizeof(TT_Entry));

  if (tt_table == NULL) {
    tt_num_entries = 0;
    return;
  }

  for (int i = 0; i < tt_num_entries; ++i) {
    atomic_init(&tt_table[i].signature, 0);
    atomic_init(&tt_table[i].payload,   0);
  }
}

void tt_clear(void) {
  if (tt_table == NULL || tt_num_entries == 0) return;

  for (int i = 0; i < tt_num_entries; ++i) {
    atomic_store_explicit(&tt_table[i].signature, 0, memory_order_relaxed);
    atomic_store_explicit(&tt_table[i].payload,   0, memory_order_relaxed);
  }
}

void tt_free(void) {
  if (tt_table != NULL) {
    free(tt_table);
    tt_table = NULL;
  }

  tt_num_entries = 0;
}

/*
 * Stores the evaluated node's score, depth, and best move into the transposition table
 * This way we can check out if the current node is already known. (probe)
 * If it is known, we prune it and save time.
 */
void tt_record(U64 hash_key, int depth, int score, TT_Flag flag, Move best_move) {
  if (tt_table == NULL || tt_num_entries == 0) return;

  int index = hash_key % tt_num_entries;
  TT_Entry *entry = &tt_table[index];

  U64 current_payload   = atomic_load_explicit(&entry->payload,   memory_order_acquire);
  U64 current_signature = atomic_load_explicit(&entry->signature, memory_order_acquire);
  bool occupied         = (current_payload & TT_OCCUPIED_MASK) != 0;
  bool same_position    = occupied && ((current_signature ^ current_payload) == hash_key);
  uint8_t current_depth = occupied ? tt_unpack_depth(current_payload) : 0;

  if (!same_position && occupied && depth < current_depth) return;

  if (best_move == 0 && same_position) {
    best_move = tt_unpack_move(current_payload);
  }

  U64 new_payload = tt_pack_payload(score, best_move, (uint8_t)depth, flag);

  atomic_store_explicit(&entry->payload,   new_payload,            memory_order_relaxed);
  atomic_store_explicit(&entry->signature, hash_key ^ new_payload, memory_order_release);
}

/*
 * Reads a coherent entry for the requested position. Depth, bound, and
 * mate-distance interpretation belong to the search, where the current ply
 * and alpha-beta window are known.
 */
bool tt_probe(U64 hash_key, TT_Data *data) {
  if (tt_table == NULL || tt_num_entries == 0) return false;

  int index = hash_key % tt_num_entries;
  TT_Entry *entry = &tt_table[index];

  U64 payload_before = atomic_load_explicit(&entry->payload,   memory_order_acquire);
  U64 signature      = atomic_load_explicit(&entry->signature, memory_order_acquire);
  U64 payload_after  = atomic_load_explicit(&entry->payload,   memory_order_acquire);

  bool invalid_entry = (payload_after & TT_OCCUPIED_MASK) == 0 || payload_before != payload_after || (signature ^ payload_after) != hash_key;
  if (invalid_entry) {
    return false;
  }

  data->score     = tt_unpack_score(payload_after);
  data->best_move = tt_unpack_move(payload_after);
  data->depth     = tt_unpack_depth(payload_after);
  data->flag      = tt_unpack_flag(payload_after);

  return true;
}

/* Takes sample from the first 1000 elements of TT and returns the fullness rate (as X per thousand) */
int tt_get_hashfull(void) {
  if (tt_table == NULL || tt_num_entries == 0) return 0;
  
  int max_samples = (tt_num_entries < 1000) ? tt_num_entries : 1000;
  int used = 0;
  
  for (int i=0; i < max_samples; ++i) {
    U64 payload = atomic_load_explicit(&tt_table[i].payload, memory_order_relaxed);
    if ((payload & TT_OCCUPIED_MASK) != 0) {
      used++;
    }
  }
  
  return (used * 1000) / max_samples;
}

static U64 tt_pack_payload(int32_t score, Move best_move, uint8_t depth, TT_Flag flag) {
  uint32_t score_bits;
  memcpy(&score_bits, &score, sizeof(score_bits));

  return ((U64)score_bits << TT_SCORE_SHIFT)
       | ((U64)best_move  << TT_MOVE_SHIFT)
       | ((U64)depth      << TT_DEPTH_SHIFT)
       | ((U64)flag       << TT_FLAG_SHIFT)
       | TT_OCCUPIED_MASK;
}

static int32_t tt_unpack_score(U64 payload) {
  uint32_t score_bits = (uint32_t)(payload >> TT_SCORE_SHIFT);
  int32_t score;
  memcpy(&score, &score_bits, sizeof(score));
  return score;
}

static Move tt_unpack_move(U64 payload) {
  return (Move)(payload >> TT_MOVE_SHIFT);
}

static uint8_t tt_unpack_depth(U64 payload) {
  return (uint8_t)(payload >> TT_DEPTH_SHIFT);
}

static TT_Flag tt_unpack_flag(U64 payload) {
  return (TT_Flag)((payload >> TT_FLAG_SHIFT) & UINT64_C(0x3));
}
