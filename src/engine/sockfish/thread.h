#pragma once

#include "sockfish.h"

typedef struct {
  SF_Context ctx;
  int thread_id;
  _Atomic U64 nodes;
} HelperThreadData;

void *helper_search_thread(void *arg);
