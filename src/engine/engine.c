#include "engine.h"
#include "sockfish/sockfish.h"
#include "sockfish/search.h"
#include "sockfish/movegen.h"
#include "board.h"
#include <SDL3/SDL.h>
#include <stdio.h>

static int engine_thread(void *data);

void engine_init(EngineWrapper *engine) {
  engine->thr                 = NULL;
  engine->mtx                 = SDL_CreateMutex();
  engine->thr_working         = false;
  engine->last_pos_hash       = 0ULL;
  engine->last_turn           = WHITE;
  engine->ctx.search_color    = WHITE;
  engine->ctx.best            = create_move(0,0);
  engine->ctx.castling_rights = CASTLE_NONE;
  engine->ctx.enpassant_sq    = NO_ENPASSANT;

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

  bool ep_valid = board->ep_row >= 0 && board->ep_col >= 0;

  make_bitboards_from_charboard(board->board, &engine->ctx);
  engine->ctx.search_color    = board->turn;
  engine->ctx.castling_rights = board->castling;
  engine->ctx.enpassant_sq    = ep_valid ? rowcol_to_sq((7 - board->ep_row), board->ep_col) : NO_ENPASSANT;

  uint64_t new_hash = position_hash(board->board, board->turn);
  if (engine->last_pos_hash == new_hash && engine->last_turn == board->turn) {
    SDL_UnlockMutex(engine->mtx);
    return;
  }
  
  engine->last_pos_hash = new_hash;
  engine->last_turn     = board->turn;
  engine->thr_working   = true;
  engine->thr           = SDL_CreateThread(engine_thread, "EngineThread", engine);
  SDL_UnlockMutex(engine->mtx);
}

static int engine_thread(void *data) {
  EngineWrapper *engine = (EngineWrapper*)(data); if (!engine) return -1;

  SF_Context ctx;

  SDL_LockMutex(engine->mtx);
  SDL_memcpy(&ctx, &engine->ctx, sizeof(SF_Context));
  SDL_UnlockMutex(engine->mtx);

  Move best = sf_search(&ctx);

  SDL_LockMutex(engine->mtx);
  engine->ctx.best = best;
  SDL_DetachThread(engine->thr);
  engine->thr         = NULL;
  engine->thr_working = false;
  SDL_UnlockMutex(engine->mtx);

  return 0;
}

void engine_destroy(EngineWrapper *engine) {
  SDL_LockMutex(engine->mtx);
  SDL_DetachThread(engine->thr); // passing NULL to SDL_DetachThread is safe (no-op)
  engine->thr = NULL;
  SDL_UnlockMutex(engine->mtx);

  SDL_DestroyMutex(engine->mtx);
  engine->mtx = NULL;

  engine->thr_working = false;
}
