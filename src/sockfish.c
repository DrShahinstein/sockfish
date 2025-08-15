#include "board.h"
#include "sockfish.h"
#include "stdlib.h"
#include <SDL3/SDL.h>

static int sf_search_thread(void *data);

static uint64_t position_hash(BoardState *b, Turn t);

void sf_init(Sockfish *sf) {
  sf->search_color   = WHITE;
  sf->thinking       = false;
  sf->best           = (Move){-1,-1,-1,-1};
  sf->thr            = NULL;
  sf->mtx            = SDL_CreateMutex();
  sf->last_pos_hash  = 0ULL;
  sf->last_turn      = WHITE;
}

void sf_req_search(Sockfish *sf, BoardState *board, Turn turn) {
  if (!sf || !board) return;

  SDL_LockMutex(sf->mtx);
  if (sf->thinking) {
    SDL_UnlockMutex(sf->mtx);
    return;
  }

  uint64_t new_hash = position_hash(board, turn);

  if (sf->last_pos_hash == new_hash && sf->last_turn == turn) {
    SDL_UnlockMutex(sf->mtx);
    return;
  }

  sf->thinking = true;
  sf->last_pos_hash = new_hash;
  sf->last_turn = turn;
  sf->thr = SDL_CreateThread(sf_search_thread, "SockfishSearchThread", sf);
  SDL_UnlockMutex(sf->mtx);
}

static int sf_search_thread(void *data) {
  Sockfish *sf = (Sockfish *)(data); if (!sf) return -1;
  
  for (int i = 0; i < 8; ++i) {
    if (i==0) SDL_Delay(500);
    SDL_Log("Sockfish searching [%d/8]", i+1);
    if (i==7) break;
    SDL_Delay(500);
  }

  SDL_LockMutex(sf->mtx);
  sf->thinking = false;
  sf->best = (Move){0,0,0,0};
  sf->search_color = sf->search_color == WHITE ? BLACK : WHITE;
  SDL_DetachThread(sf->thr);
  sf->thr = NULL;
  SDL_UnlockMutex(sf->mtx);

  return 0;
}

static uint64_t position_hash(BoardState *b, Turn t) {
  const uint64_t FNV_OFFSET = 1469598103934665603ULL;
  const uint64_t FNV_PRIME = 1099511628211ULL;
  uint64_t h = FNV_OFFSET;

  for (int r = 0; r < 8; ++r) {
    for (int c = 0; c < 8; ++c) {
      unsigned char v = (unsigned char)b->board[r][c];
      h ^= (uint64_t)v;
      h *= FNV_PRIME;
    }
  }

  h ^= (uint64_t)t;
  h *= FNV_PRIME;
  return h;
}

void sf_destroy(Sockfish *sf) {
  SDL_LockMutex(sf->mtx);
  if (sf->thr != NULL)
  {
    SDL_DetachThread(sf->thr);
    sf->thr = NULL;
  }
  SDL_UnlockMutex(sf->mtx);

  SDL_DestroyMutex(sf->mtx);
}
