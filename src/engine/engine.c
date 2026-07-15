#include "engine.h"
#include "board.h"
#include "sockfish/sockfish.h"
#include "sockfish/search.h"
#include "sockfish/transposition_table.h"
#include "sockfish/config.h"
#include <SDL3/SDL.h>

static int engine_thread(void *data);

void engine_init(EngineWrapper *engine) {
  config_load(SOCKFISH_INI, &engine->active_config);
  tt_init(engine->active_config.tt_size_mb); // init transposition table with configured MB

  engine->mtx              = SDL_CreateMutex();
  engine->cond             = SDL_CreateCondition();
  engine->last_pos_hash    = 0;
  engine->ctx              = create_sf_ctx(&(BitboardSet){0}, WHITE, CASTLE_ALL, NO_ENPASSANT);
  engine->pending_config   = engine->active_config;
  engine->config_changed   = false;
  engine->should_stop      = false;
  engine->abort_search     = false;
  engine->pending_tt_clear = false;
  engine->thr_working      = false;
  engine->thr              = SDL_CreateThread(engine_thread, "EngineThread", engine);
}

void engine_update_config(EngineWrapper *engine, const SF_Config *new_config) {
  SDL_LockMutex(engine->mtx);
  
  engine->pending_config = *new_config;
  engine->config_changed = true;

  if (engine->thr_working) {
    engine->abort_search = true;
  }
  
  SDL_UnlockMutex(engine->mtx);
}

void engine_req_search(EngineWrapper *engine, const BoardState *board) {
  if (board->promo.active) return;

  SDL_LockMutex(engine->mtx);

  U64 hash_now    = board->position_hash;
  U64 hash_before = engine->last_pos_hash;
  bool same       = hash_now == hash_before;

  if (same) {
    SDL_UnlockMutex(engine->mtx);
    return;
  }

  if (engine->thr_working) {
    engine->abort_search = true;
    SDL_UnlockMutex(engine->mtx);
    return;
  }

  if (engine->pending_tt_clear) {
    tt_clear();
    engine->pending_tt_clear = false;
  }

  bool ep_valid     = board->ep_row >= 0 && board->ep_col >= 0;
  BitboardSet bbset = make_bitboards_from_charboard(board->board);
  Square en_passant = ep_valid ? rowcol_to_sq(board->ep_row, board->ep_col) : NO_ENPASSANT;
  SF_Context ctx    = create_sf_ctx(&bbset, board->turn, board->castling, en_passant);

  /* Make position history for Sockfish by transferring the one that BoardState already has  */
  ctx.history_count = board->undo_count;
  for (int i=0; i < board->undo_count; ++i) {
    ctx.pos_history[i] = board->hash_history[i];
  }

  engine->ctx             = ctx;
  engine->ctx.should_stop = &engine->abort_search;
  engine->abort_search    = false;
  engine->last_pos_hash   = hash_now;
  engine->thr_working     = true;

  SDL_SignalCondition(engine->cond);
  SDL_UnlockMutex(engine->mtx);
}

void engine_request_tt_clear(EngineWrapper *engine) {
  SDL_LockMutex(engine->mtx);
  engine->pending_tt_clear = true;
  SDL_UnlockMutex(engine->mtx);
}

void engine_abort_search(EngineWrapper *engine) {
  SDL_LockMutex(engine->mtx);
  engine->abort_search = true;
  if (engine->thr_working) engine->last_pos_hash = 0;
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

    if (engine->config_changed) {
      if (engine->active_config.tt_size_mb != engine->pending_config.tt_size_mb) {
        tt_free();
        tt_init(engine->pending_config.tt_size_mb);
      }
      
      engine->active_config  = engine->pending_config;
      engine->config_changed = false;
    }

    SF_Context ctx = engine->ctx;
    ctx.time_limit = engine->active_config.move_time_ms;

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
}

