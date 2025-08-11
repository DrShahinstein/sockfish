#pragma once

#include <SDL3/SDL.h>

typedef struct GameState GameState;
typedef enum { WHITE, BLACK } Turn;
typedef struct Sockfish Sockfish;

Sockfish *sf_create();
void sf_destroy(Sockfish *sf);
