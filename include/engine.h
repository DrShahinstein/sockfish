#pragma once

#include "sockfish.h"
#include <SDL3/SDL.h>

typedef struct BoardState BoardState;

typedef struct EngineWrapper {
  SDL_Mutex *mtx;
  SDL_Thread *thr;
  SF_Context ctx;
  uint64_t last_pos_hash;
  Turn last_turn;
} EngineWrapper;

void engine_init(EngineWrapper *engine);
void engine_req_search(EngineWrapper *engine, const BoardState *board);
void engine_destroy(EngineWrapper *engine);