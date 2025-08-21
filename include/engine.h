#pragma once

#include "board.h"
#include <SDL3/SDL.h>

typedef struct Sockfish {
  SDL_Mutex *mtx;
  SDL_Thread *thr;
  Turn search_color;
  Move best;
  bool thinking;
  uint64_t last_pos_hash;
  Turn last_turn;
} Sockfish;

void sf_init(Sockfish *sf);
void sf_req_search(Sockfish *sf, BoardState *board);
void sf_destroy(Sockfish *sf);