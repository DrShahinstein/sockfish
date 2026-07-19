#pragma once

#include "sockfish.h"
#include <pthread.h>
#include <stdatomic.h>

typedef struct {
  pthread_t thread;
  bool thread_valid;
  atomic_bool running;
  volatile bool stop_flag;
  SF_Context ctx;
} AsyncSearch;

void uci_loop(void);

