#pragma once

#include "sockfish/sockfish.h"

Move sf_search(const SF_Context *ctx);

int negamax(SF_Context *ctx, unsigned int depth, int ply, int alpha, int beta);

int quiescence_search(SF_Context *ctx, int ply, int alpha, int beta);
