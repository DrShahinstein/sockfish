#include "sockfish.h"
#include "stdlib.h"
#include <SDL3/SDL.h>

static int sf_search_thread(void *data);

void sf_init(Sockfish *sf) {
  sf->search_color = WHITE;
  sf->thinking     = false;
  sf->best         = (Move){-1,-1,-1,-1};
  sf->thr          = NULL;
  sf->mtx          = SDL_CreateMutex();
}

void sf_req_search(Sockfish *sf) {
  SDL_LockMutex(sf->mtx);
  if (sf->thinking) {
    SDL_UnlockMutex(sf->mtx);
    return;
  }

  sf->thinking = true;
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
  sf->search_color = BLACK;
  sf->best = (Move){0,0,0,0};
  sf->thinking = false;
  SDL_DetachThread(sf->thr);
  SDL_UnlockMutex(sf->mtx);

  return 0;
}

void sf_destroy(Sockfish *sf) {
  SDL_LockMutex(sf->mtx);
  SDL_DetachThread(sf->thr);
  sf->thr = NULL;
  SDL_UnlockMutex(sf->mtx);
}
