#pragma once

#include "sockfish/sockfish.h"
#include <SDL3/SDL.h>

typedef struct BoardState BoardState;

typedef struct EngineWrapper {
  SDL_Mutex *mtx;
  SDL_Thread *thr;
  SDL_Condition *cond;
  SF_Context ctx;
  uint64_t last_pos_hash;
  bool thr_working;
  bool should_stop;
  bool abort_search;
  bool pending_tt_clear;
} EngineWrapper;

void engine_init(EngineWrapper *engine);
void engine_req_search(EngineWrapper *engine, const BoardState *board);
void engine_request_tt_clear(EngineWrapper *engine);
void engine_abort_search(EngineWrapper *engine);
void engine_destroy(EngineWrapper *engine);

BitboardSet make_bitboards_from_charboard(const char board[8][8]); // bitboard_maker.c

/*

  EngineWrapper: A wrapper for the chess engine context and threading.
  It provides a bridge between the main application and the chess engine (in this case, Sockfish).
  This way, Sockfish is completely isolated from the rest of the application. Being a standalone package on its own.

  So, 'Engine' and 'Sockfish' are two different concepts.
  'Engine' is a constant bridge module in this application, which connects 'Sockfish' into the project.
  And, 'Sockfish' is a standalone chess engine module. Regards search algorithms, depth and so on...

*/
