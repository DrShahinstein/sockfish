#include "sockfish.h"
#include "stdlib.h"
#include "ui.h" // MAX_FEN


struct SF_Move {

};

struct Sockfish {
  bool engine_on;
  Turn search_color;
  SDL_Thread *thread;
  SDL_AtomicInt searching;
  SDL_AtomicInt stop_req;
  SDL_Mutex *lock;
  char fen[MAX_FEN];
  SF_Move best_move;
};

Sockfish *sf_create() {
  Sockfish *sf = (Sockfish *)malloc(sizeof(Sockfish));
  if (!sf) return NULL;
  sf->thread = NULL;
  sf->lock = SDL_CreateMutex();
  SDL_SetAtomicInt(&sf->searching, 0);
  SDL_SetAtomicInt(&sf->stop_req, 0);
  sf->fen[0] = '\0';
  sf->search_color = WHITE;
  return sf;
}

void sf_destroy(Sockfish *sf) {
  if (!sf) return;
  // sf_stop_search(sf);
  if (sf->lock) SDL_DestroyMutex(sf->lock);
  free(sf);
}
