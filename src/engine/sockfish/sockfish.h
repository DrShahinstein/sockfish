#pragma once

#include "bitboard.h"
#include <stdbool.h>

typedef enum {
  WHITE, BLACK
} Turn;

typedef struct {
  int fr; int fc;
  int tr; int tc;
} Move;

typedef struct SF_Context {
  BitboardSet bitboard_set;
  Turn search_color;
  Move best;
  bool thinking;
} SF_Context;

Move sf_search(const SF_Context *ctx); // search.c