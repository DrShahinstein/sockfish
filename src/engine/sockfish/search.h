#pragma once

#include "sockfish/sockfish.h"

typedef struct {
  U64 pawn;
  U64 knight;
  U64 bishop;
  U64 rook;
} CheckMasks;

Move sf_search(const SF_Context *ctx);

int negamax(SF_Context *ctx, unsigned int depth, int ply, int alpha, int beta);

int quiescence_search(SF_Context *ctx, int ply, int alpha, int beta);
