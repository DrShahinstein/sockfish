#include "transposition_table.h"
#include <stdlib.h>
#include <string.h>

TT_Entry *tt_table = NULL; // this is the transposition table
int tt_num_entries = 0;

void tt_init(int size_mb) {
  if (tt_table != NULL) {
    tt_free();
  }

  size_t bytes   = (size_t)size_mb * 1024 * 1024;
  tt_num_entries = bytes / sizeof(TT_Entry);

  tt_table = (TT_Entry *)calloc(tt_num_entries, sizeof(TT_Entry));

  if (tt_table == NULL)
    tt_num_entries = 0;
}

void tt_clear(void) {
  if (tt_table != NULL && tt_num_entries > 0)
    memset(tt_table, 0, (size_t)tt_num_entries * sizeof(TT_Entry));
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

  if (entry->hash_key == hash_key || depth >= entry->depth) {
    entry->hash_key = hash_key;
    entry->score    = score;
    entry->flag     = flag;
    entry->depth    = depth;

    if (best_move != 0 || entry->hash_key != hash_key) {
      entry->best_move = best_move;
    }
  }
}

/* 
 * Queries the record of the current position in the table.
 * If the position has been calculated previously,
 * it assigns that position's score to 'return_score' and returns 'true' to prune the branch.
 */
bool tt_probe(U64 hash_key, int depth, int alpha, int beta, int *return_score, Move *best_move) {
  if (tt_table == NULL || tt_num_entries == 0) return false;

  int index = hash_key % tt_num_entries;
  TT_Entry *entry = &tt_table[index];

  if (entry->hash_key == hash_key) {
    *best_move = entry->best_move;

    if (entry->depth >= depth) {
      int score = entry->score;

      if (entry->flag == TT_EXACT) {
        *return_score = score;
        return true;
      }

      if (entry->flag == TT_ALPHA && score <= alpha) {
        *return_score = alpha;
        return true;
      }

      if (entry->flag == TT_BETA && score >= beta) {
        *return_score = beta;
        return true;
      }
    }
  }

  return false; 
}
