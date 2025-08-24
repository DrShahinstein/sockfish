#pragma once

#include "sockfish.h"
#include <SDL3/SDL.h>

typedef struct BoardState BoardState;

typedef struct EngineWrapper {
  SDL_Mutex *mtx;
  SDL_Thread *thr;
  SF_Context ctx;
  uint64_t last_pos_hash;
  Turn last_turn;
} EngineWrapper;

void engine_init(EngineWrapper *engine);
void engine_req_search(EngineWrapper *engine, const BoardState *board);
void engine_destroy(EngineWrapper *engine);

/*

  EngineWrapper: A wrapper for the chess engine context and threading.
  It provides a bridge between the main application and the chess engine (in this case, Sockfish).
  This way, Sockfish is completely isolated from the rest of the application. Being a standalone package on its own.

  - mtx: Mutex for thread safety.
  - thr: Thread handle for the engine's search thread.
  - ctx: The Sockfish engine context, holding the board state and search parameters. (!)
  - last_pos_hash: Hash of the last searched position to avoid redundant searches.
  - last_turn: The turn color of the last searched position.

  So, 'Engine' and 'Sockfish' are two different concepts.
  'Engine' is a constant bridge module in this application, which connects 'Sockfish' into the project.
  And, 'Sockfish' is a standalone chess engine module. Regards search algorithms, depth and so on...

*/