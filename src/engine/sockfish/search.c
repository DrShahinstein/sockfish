#include "sockfish/search.h"

/* not yet implemented */
Move sf_search(const SF_Context *ctx) {
  (void)ctx;

  return create_move(E2, E4);
}



/*

NOTE for myself:

You can take advantage of stop_requested available in SF_Context to manage graceful shutdown.

if (ctx->stop_requested && *ctx->stop_requested){
}

*/