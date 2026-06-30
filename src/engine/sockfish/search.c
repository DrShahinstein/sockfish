#include "sockfish/search.h"
#include "sockfish/evaluation.h"
#include "sockfish/sockfish.h"

#include <SDL3/SDL.h> // DEBUG

#define INF 9999999

Move sf_search(const SF_Context *ctx) {
  (void)ctx;
  return create_move(A1,A1);
}
