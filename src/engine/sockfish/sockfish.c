#include "sockfish.h"
#include "evaluation.h"  // sf_init_evaluation()
#include <string.h>

SF_Context create_sf_ctx(BitboardSet *bitboard_set, Turn search_color, uint8_t castling_rights, Square ep_sq) {
  SF_Context ctx;
  ctx.bitboard_set    = *bitboard_set;
  ctx.search_color    = search_color;
  ctx.castling_rights = castling_rights;
  ctx.enpassant_sq    = ep_sq;
  ctx.should_stop     = NULL;
  ctx.nodes           = 0;
  ctx.start_time      = 0;
  ctx.time_limit      = 0;
  ctx.hash_key        = 0;
  ctx.history_count   = 0;
  ctx.best            = create_move(0,0);

  memset(ctx.killer_moves,      0, sizeof(ctx.killer_moves));
  memset(ctx.history_heuristic, 0, sizeof(ctx.history_heuristic));

  sf_init_hash_key(&ctx);
  sf_init_evaluation(&ctx);

  return ctx;
}

