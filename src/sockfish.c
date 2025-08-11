#include "sockfish.h"
#include "stdlib.h"
#include <SDL3/SDL.h>

void sf_init(Sockfish *sf) {
  (void)sf;
  SDL_Log("hi");
}
void sf_destroy(Sockfish *sf) {
  (void)sf;
  SDL_Log("bye");
}
