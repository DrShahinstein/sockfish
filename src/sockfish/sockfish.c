#include "sockfish.h"
#include <SDL3/SDL.h> // temp

Move sf_search(const SF_Context *ctx) {
  (void)ctx;

  int a = SDL_rand(8) + 1;
  int b = SDL_rand(8) + 1;
  int c = SDL_rand(8) + 1;
  int d = SDL_rand(8) + 1;

  for (int i = 0; i < 4; ++i) {
    if (i==0) SDL_Delay(100);
    SDL_Log("Searching for %s... [%d/4]", ctx->search_color == WHITE ? "White" : "Black", i+1);
    if (i!=3) SDL_Delay(100);
  }

  return (Move){a,b,c,d};
}