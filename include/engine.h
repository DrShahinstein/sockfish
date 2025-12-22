#pragma once

#include "sockfish/sockfish.h"
#include <SDL3/SDL.h>

typedef struct BoardState BoardState;

typedef struct EngineWrapper {
  SDL_Mutex *mtx;
  SDL_Thread *thr;
  SDL_Condition *cond;
  bool thr_working;
  bool should_stop;
  SF_Context ctx;
  uint64_t last_pos_hash;
  Turn last_turn;
} EngineWrapper;

void engine_init(EngineWrapper *engine);
void engine_req_search(EngineWrapper *engine, const BoardState *board);
void engine_destroy(EngineWrapper *engine);

BitboardSet make_bitboards_from_charboard(const char board[8][8]); // bitboard_maker.c
uint64_t position_hash(const char b[8][8], Turn t);                // position_hasher.c

/*

  EngineWrapper: A wrapper for the chess engine context and threading.
  It provides a bridge between the main application and the chess engine (in this case, Sockfish).
  This way, Sockfish is completely isolated from the rest of the application. Being a standalone package on its own.

  So, 'Engine' and 'Sockfish' are two different concepts.
  'Engine' is a constant bridge module in this application, which connects 'Sockfish' into the project.
  And, 'Sockfish' is a standalone chess engine module. Regards search algorithms, depth and so on...

*/
