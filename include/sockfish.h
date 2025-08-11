#pragma once

#include <stdbool.h>

typedef struct GameState GameState;

typedef enum {
  WHITE, BLACK
} Turn;

typedef struct Sockfish {
  bool engine_on;
  Turn turn;

} Sockfish;

void sf_init(GameState *game);
void sf_destroy(GameState *game);
