#pragma once

#include <SDL3/SDL.h>

typedef struct BoardState BoardState;

typedef enum { WHITE, BLACK } Turn;

typedef struct Move {
  int fr; int fc;
  int tr; int tc;
} Move;

typedef struct Sockfish {
  SDL_Mutex *mtx;
  SDL_Thread *thr;
  char *board_ref;
  Turn search_color;
  Move best;
  bool thinking;
  uint64_t last_pos_hash;
  Turn last_turn;
} Sockfish;

void sf_init(Sockfish *sf);
void sf_req_search(Sockfish *sf, BoardState *board, Turn turn);
void sf_destroy(Sockfish *sf);