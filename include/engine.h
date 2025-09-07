#pragma once

#include "sockfish/sockfish.h"
#include <SDL3/SDL.h>

typedef struct BoardState BoardState;

typedef struct EngineWrapper {
  SDL_Mutex *mtx;
  SDL_Thread *thr;
  bool thr_working;
  SF_Context ctx;
  uint64_t last_pos_hash;
  Turn last_turn;
} EngineWrapper;

void engine_init(EngineWrapper *engine);
void engine_req_search(EngineWrapper *engine, const BoardState *board);
void engine_destroy(EngineWrapper *engine);

void make_bitboards_from_charboard(const char board[8][8], SF_Context *ctx); // bitboard_maker.c
uint64_t position_hash(const char b[8][8], Turn t);                          // position_hasher.c

/*

  EngineWrapper: A wrapper for the chess engine context and threading.
  It provides a bridge between the main application and the chess engine (in this case, Sockfish).
  This way, Sockfish is completely isolated from the rest of the application. Being a standalone package on its own.

  - mtx: Mutex for thread safety.
  - thr: Thread handle for the engine's search thread.
  - ctx: The Sockfish engine context, holding the board state and search parameters. (!)
  - last_pos_hash: Hash of the last searched position to avoid redundant searches.
  - last_turn: The turn color of the last searched position.

  So, 'Engine' and 'Sockfish' are two different concepts.
  'Engine' is a constant bridge module in this application, which connects 'Sockfish' into the project.
  And, 'Sockfish' is a standalone chess engine module. Regards search algorithms, depth and so on...

*/