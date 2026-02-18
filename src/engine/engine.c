#include "engine.h"
#include "board.h"
#include "sockfish/sockfish.h"
#include "sockfish/search.h"
#include "sockfish/movegen.h"
#include <SDL3/SDL.h>

static int engine_thread(void *data);

void engine_init(EngineWrapper *engine) {
  /* init precomputed attack tables for sockfish's move generation logic */
  init_attack_tables();

  /* init magic bitboards for sliding pieces in move generation logic */
  init_magic_bitboards();

  engine->mtx           = SDL_CreateMutex();
  engine->cond          = SDL_CreateCondition();
  engine->last_pos_hash = 0;
  engine->ctx           = create_sf_ctx(&(BitboardSet){0}, WHITE, CASTLE_ALL, NO_ENPASSANT);
  engine->should_stop   = false;
  engine->abort_search  = false;
  engine->thr_working   = false;
  engine->thr           = SDL_CreateThread(engine_thread, "EngineThread", engine);
}

void engine_req_search(EngineWrapper *engine, const BoardState *board) {
  if (board->promo.active) return;

  SDL_LockMutex(engine->mtx);

  uint64_t hash_now    = board->position_hash;
  uint64_t hash_before = engine->last_pos_hash;
  bool same            = hash_now == hash_before;

  if (same) {
    SDL_UnlockMutex(engine->mtx);
    return;
  }

  if (engine->thr_working) {
    engine->abort_search = true;
    SDL_UnlockMutex(engine->mtx);
    return;
  }

  bool ep_valid     = board->ep_row >= 0 && board->ep_col >= 0;
  BitboardSet bbset = make_bitboards_from_charboard(board->board);
  Square en_passant = ep_valid ? rowcol_to_sq(board->ep_row, board->ep_col) : NO_ENPASSANT;
  SF_Context ctx    = create_sf_ctx(&bbset, board->turn, board->castling, en_passant);

  engine->ctx             = ctx;
  engine->ctx.should_stop = &engine->abort_search;
  engine->abort_search    = false;
  engine->last_pos_hash   = hash_now;
  engine->thr_working     = true;

  SDL_SignalCondition(engine->cond);
  SDL_UnlockMutex(engine->mtx);
}

static int engine_thread(void *data) {
  EngineWrapper *engine = data;
  if (engine == NULL) return -1;

  for (;;) {
    SDL_LockMutex(engine->mtx);

    while (!engine->thr_working && !engine->should_stop) {
      SDL_WaitCondition(engine->cond, engine->mtx);
    }

    if (engine->should_stop) {
      SDL_UnlockMutex(engine->mtx);
      break;
    }

    SF_Context ctx = engine->ctx;
    SDL_UnlockMutex(engine->mtx);

    Move best = sf_search(&ctx);

    SDL_LockMutex(engine->mtx);
    engine->ctx.best    = best;
    engine->thr_working = false;
    SDL_UnlockMutex(engine->mtx);
  }

  return 0;
}

void engine_destroy(EngineWrapper *engine) {
  SDL_LockMutex(engine->mtx);
  engine->should_stop  = true;
  engine->abort_search = true;
  SDL_SignalCondition(engine->cond);
  SDL_UnlockMutex(engine->mtx);

  SDL_WaitThread(engine->thr, NULL);
  SDL_DestroyCondition(engine->cond);
  SDL_DestroyMutex(engine->mtx);

  engine->thr  = NULL;
  engine->mtx  = NULL;
  engine->cond = NULL;

  cleanup_magic_bitboards();
}
