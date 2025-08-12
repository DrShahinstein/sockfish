#pragma once

#include <SDL3/SDL.h>

typedef enum { WHITE, BLACK } Turn;
typedef struct Sockfish {
  Turn search_color;
} Sockfish;

void sf_init(Sockfish *sf);
void sf_destroy(Sockfish *sf);
