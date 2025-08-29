#include "engine.h"

uint64_t position_hash(const char b[8][8], Turn t) {
  const uint64_t FNV_OFFSET = 14695981039346656037ULL;
  const uint64_t FNV_PRIME = 1099511628211ULL;
  uint64_t h = FNV_OFFSET;

  for (int r = 0; r < 8; ++r) {
    for (int c = 0; c < 8; ++c) {
      unsigned char v = (unsigned char)b[r][c];
      h ^= v;
      h *= FNV_PRIME;
      h ^= (r << 4 | c);
      h *= FNV_PRIME;
    }
  }

  h ^= (uint64_t)t;
  h *= FNV_PRIME;
  
  return h;
}