#include "sockfish.h"
#include <SDL3/SDL.h> // temp

Move sf_search(SF_Context *ctx) {
  (void)ctx;

  for (int i = 0; i < 4; ++i) {
    if (i==0) SDL_Delay(100);
    SDL_Log("Searching for %s... [%d/4]", ctx->search_color == WHITE ? "White" : "Black", i+1);
    if (i!=3) SDL_Delay(100);
  }

  return (Move){0};
}