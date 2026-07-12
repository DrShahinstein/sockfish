#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include "platform.h"

#if defined(_WIN32) || defined(_WIN64)
  #include <windows.h>
  U64 get_time_ms(void) {
      return (U64)GetTickCount64();
  }
#else
  #include <time.h>
  U64 get_time_ms(void) {
      struct timespec ts;
      clock_gettime(CLOCK_MONOTONIC, &ts);
      return (U64)(ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL);
  }
#endif

