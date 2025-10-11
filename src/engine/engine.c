#include "engine.h"
#include "sockfish/sockfish.h"
#include "sockfish/search.h"
#include "sockfish/movegen.h"
#include "board.h"
#include <SDL3/SDL.h>
#include <stdio.h>

static int engine_thread(void *data);

void engine_init(EngineWrapper *engine) {
  engine->thr                 = SDL_CreateThread(engine_thread, "EngineThread", engine);
  engine->mtx                 = SDL_CreateMutex();
  engine->cond                = SDL_CreateCondition();
  engine->thr_working         = false;
  engine->stop_requested      = false;
  engine->last_pos_hash       = 0ULL;
  engine->last_turn           = WHITE;
  engine->ctx                 = create_sf_ctx(&(BitboardSet){0}, WHITE, CASTLE_NONE, NO_ENPASSANT);

  init_attack_tables();   // init precomputed attack tables for sockfish's move generation logic
  init_magic_bitboards(); // init magic bitboards for sliding pieces in move generation logic
}

void engine_req_search(EngineWrapper *engine, const BoardState *board) {
  if (!engine) return;
  if (board->promo.active) return;

  SDL_LockMutex(engine->mtx);
  if (engine->thr_working) {
    SDL_UnlockMutex(engine->mtx);
    return;
  }

  uint64_t new_hash = position_hash(board->board, board->turn);
  if (engine->last_pos_hash == new_hash && engine->last_turn == board->turn) {
    SDL_UnlockMutex(engine->mtx);
    return;
  }

  bool ep_valid = board->ep_row >= 0 && board->ep_col >= 0;
  
  BitboardSet bbset = make_bitboards_from_charboard(board->board);
  Square en_passant = ep_valid ? rowcol_to_sq_for_engine(board->ep_row, board->ep_col) : NO_ENPASSANT;
  SF_Context ctx    = create_sf_ctx(&bbset, board->turn, board->castling, en_passant);

  engine->ctx           = ctx;
  engine->last_pos_hash = new_hash;
  engine->last_turn     = board->turn;
  engine->thr_working   = true;
  
  SDL_SignalCondition(engine->cond);
  SDL_UnlockMutex(engine->mtx);
}

static int engine_thread(void *data) {
  EngineWrapper *engine = (EngineWrapper *)(data);
  if (!engine) return -1;

  while (1) {
    SDL_LockMutex(engine->mtx);

    while (!engine->thr_working && !engine->stop_requested) {
      SDL_WaitCondition(engine->cond, engine->mtx);
    }

    if (engine->stop_requested) {
      SDL_UnlockMutex(engine->mtx);
      break;
    }

    SF_Context ctx;
    SDL_memcpy(&ctx, &engine->ctx, sizeof(SF_Context));
    ctx.stop_requested = &engine->stop_requested;
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
  engine->stop_requested = true;
  SDL_SignalCondition(engine->cond);
  SDL_UnlockMutex(engine->mtx);

  SDL_WaitThread(engine->thr, NULL);
  SDL_DestroyCondition(engine->cond);
  SDL_DestroyMutex(engine->mtx);

  engine->thr         = NULL;
  engine->mtx         = NULL;
  engine->cond        = NULL;
  engine->thr_working = false;

  cleanup_magic_bitboards();
}