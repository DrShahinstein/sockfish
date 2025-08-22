#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum { WHITE, BLACK } Turn;

typedef struct {
  int fr; int fc;
  int tr; int tc;
} Move;

typedef struct SF_Context {
  char board_ref[8][8];
  Turn search_color;
  Move best;
  bool thinking;
} SF_Context;

Move sf_search(SF_Context *ctx);