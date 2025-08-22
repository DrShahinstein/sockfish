#pragma once

#include "sockfish.h"
#include <SDL3/SDL.h>

typedef struct BoardState BoardState;

typedef struct Engine {
  SDL_Mutex *mtx;
  SDL_Thread *thr;
  SF_Context ctx;
  uint64_t last_pos_hash;
  Turn last_turn;
} Engine;

void engine_init(Engine *engine);
void engine_req_search(Engine *engine, const BoardState *board);
void engine_destroy(Engine *engine);