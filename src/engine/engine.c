#include "engine.h"
#include "board.h"
#include <SDL3/SDL.h>

static int engine_thread(void *data);
static uint64_t position_hash(const char b[8][8], Turn t);

void engine_init(EngineWrapper *engine) {
  engine->thr               = NULL;
  engine->mtx               = SDL_CreateMutex();
  engine->last_pos_hash     = 0ULL;
  engine->last_turn         = WHITE;
  engine->ctx.search_color  = WHITE;
  engine->ctx.best          = (Move){-1,-1,-1,-1};
  engine->ctx.thinking      = false;
  SDL_memset(engine->ctx.board_ref, 0, sizeof(engine->ctx.board_ref));
}

void engine_req_search(EngineWrapper *engine, const BoardState *board) {
  if (!engine) return;
  if (board->promo.active) return;

  SDL_LockMutex(engine->mtx);

  // return if already thinking
  if (engine->ctx.thinking) {
    SDL_UnlockMutex(engine->mtx);
    return;
  }

  // make board_ref from board->board
  SDL_memcpy(engine->ctx.board_ref, board->board, sizeof(engine->ctx.board_ref));

  // match search color with actual turn on board
  engine->ctx.search_color = board->turn;

  // create position hash to avoid searching for the same position
  uint64_t new_hash = position_hash((const char (*)[8])engine->ctx.board_ref, board->turn);
  if (engine->last_pos_hash == new_hash && engine->last_turn == board->turn) {
    SDL_UnlockMutex(engine->mtx);
    return;
  }
  
  // search thread is about to spawn
  engine->last_pos_hash = new_hash;
  engine->last_turn = board->turn;
  engine->ctx.thinking = true;
  engine->thr = SDL_CreateThread(engine_thread, "EngineThread", engine);
  SDL_UnlockMutex(engine->mtx);
}

static int engine_thread(void *data) {
  EngineWrapper *engine = (EngineWrapper*)(data); if (!engine) return -1;

  SF_Context ctx = engine->ctx;

  SDL_LockMutex(engine->mtx);
  SDL_memcpy(&ctx, &engine->ctx, sizeof(SF_Context));
  SDL_UnlockMutex(engine->mtx);

  Move best = sf_search(&ctx);

  SDL_LockMutex(engine->mtx);
  engine->ctx.best = best;
  SDL_DetachThread(engine->thr);
  engine->thr = NULL;
  engine->ctx.thinking = false;
  SDL_UnlockMutex(engine->mtx);

  return 0;
}

static uint64_t position_hash(const char b[8][8], Turn t) {
  const uint64_t FNV_OFFSET = 14695981039346656037ULL;
  const uint64_t FNV_PRIME = 1099511628211ULL;
  uint64_t h = FNV_OFFSET;

  for (int r = 0; r < 8; ++r) {
    for (int c = 0; c < 8; ++c) {
      unsigned char v = (unsigned char)b[r][c];
      h ^= v;
      h *= FNV_PRIME;
      h ^= (r << 4 | c);
      h *= FNV_PRIME;
    }
  }

  h ^= (uint64_t)t;
  h *= FNV_PRIME;
  
  return h;
}

void engine_destroy(EngineWrapper *engine) {
  SDL_LockMutex(engine->mtx);
  SDL_DetachThread(engine->thr); // passing NULL to SDL_DetachThread is safe (no-op)
  engine->thr = NULL;
  SDL_UnlockMutex(engine->mtx);

  SDL_DestroyMutex(engine->mtx);
}
