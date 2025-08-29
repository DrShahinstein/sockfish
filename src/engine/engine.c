#include "engine.h"
#include "board.h"
#include <SDL3/SDL.h>

static int engine_thread(void *data);
static uint64_t position_hash(const char b[8][8], Turn t);
static void make_bitboards_from_charboard(const char board[8][8], SF_Context *ctx);

void engine_init(EngineWrapper *engine) {
  engine->thr               = NULL;
  engine->mtx               = SDL_CreateMutex();
  engine->last_pos_hash     = 0ULL;
  engine->last_turn         = WHITE;
  engine->ctx.search_color  = WHITE;
  engine->ctx.best          = (Move){-1,-1,-1,-1};
  engine->ctx.thinking      = false;
}

void engine_req_search(EngineWrapper *engine, const BoardState *board) {
  if (!engine) return;
  if (board->promo.active) return;

  SDL_LockMutex(engine->mtx);
  if (engine->ctx.thinking) {
    SDL_UnlockMutex(engine->mtx);
    return;
  }

  make_bitboards_from_charboard(board->board, &engine->ctx);
  engine->ctx.search_color = board->turn;

  uint64_t new_hash = position_hash(board->board, board->turn);
  if (engine->last_pos_hash == new_hash && engine->last_turn == board->turn) {
    SDL_UnlockMutex(engine->mtx);
    return;
  }
  
  engine->last_pos_hash = new_hash;
  engine->last_turn     = board->turn;
  engine->ctx.thinking  = true;
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

static void make_bitboards_from_charboard(const char board[8][8], SF_Context *ctx) {
  for (int color = 0; color < 2; ++color) {
    ctx->bitboard_set.pawns     [color] = 0;
    ctx->bitboard_set.knights   [color] = 0;
    ctx->bitboard_set.bishops   [color] = 0;
    ctx->bitboard_set.rooks     [color] = 0;
    ctx->bitboard_set.queens    [color] = 0;
    ctx->bitboard_set.kings     [color] = 0;
    ctx->bitboard_set.all_pieces[color] = 0;
  }
  ctx->bitboard_set.occupied = 0;

  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      char piece = board[row][col];
      if (piece == 0)
        continue;

      int square = (7 - row) * 8 + col;
      int color  = (piece >= 'A' && piece <= 'Z') ? WHITE : BLACK;
      char piece_lower = SDL_tolower(piece);

      SET_BIT(ctx->bitboard_set.occupied, square);
      SET_BIT(ctx->bitboard_set.all_pieces[color], square);

      switch (piece_lower) {
       case 'p': SET_BIT(ctx->bitboard_set.pawns  [color], square); break;
       case 'n': SET_BIT(ctx->bitboard_set.knights[color], square); break;
       case 'b': SET_BIT(ctx->bitboard_set.bishops[color], square); break;
       case 'r': SET_BIT(ctx->bitboard_set.rooks  [color], square); break;
       case 'q': SET_BIT(ctx->bitboard_set.queens [color], square); break;
       case 'k': SET_BIT(ctx->bitboard_set.kings  [color], square); break;
      }
    }
  }
}

void engine_destroy(EngineWrapper *engine) {
  SDL_LockMutex(engine->mtx);
  SDL_DetachThread(engine->thr); // passing NULL to SDL_DetachThread is safe (no-op)
  engine->thr = NULL;
  SDL_UnlockMutex(engine->mtx);

  SDL_DestroyMutex(engine->mtx);
}
