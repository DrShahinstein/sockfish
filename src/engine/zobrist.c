#include "engine.h"
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

uint64_t zobrist_pieces[12][64];
uint64_t zobrist_black_to_move;

static uint64_t rand64(void);
static inline int piece_to_index(char piece);

void init_zobrist_keys(void) {
  for (int piece = 0; piece < 12; piece++) {
    for (int square = 0; square < 64; square++) {
      zobrist_pieces[piece][square] = rand64();
    }
  }
  
  zobrist_black_to_move = rand64();
}

uint64_t zobrist_hash(const char board[8][8], Turn turn) {
  uint64_t hash = 0;
  
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      char piece = board[row][col];
      if (piece != 0) {
        int piece_idx = piece_to_index(piece);
        if (piece_idx >= 0) {
          Square s = rowcol_to_sq(row, col);
          hash ^= zobrist_pieces[piece_idx][s];
        }
      }
    }
  }
  
  if (turn == BLACK) {
    hash ^= zobrist_black_to_move;
  }
  
  return hash;
}

static uint64_t rand64(void) {
  static bool seeded = false;
  
  if (!seeded) {
    uint64_t seed_val = SDL_GetPerformanceCounter();
    SDL_srand((unsigned int)(seed_val & 0xFFFFFFFF));
    seeded = true;
  }
  
  uint64_t result = 0;
  for (int i = 0; i < 8; i++) {
    result <<= 8;
    result |= (uint8_t)SDL_rand(256);
  }
  
  return result;
}

static inline int piece_to_index(char piece) {
  switch (piece) {
    case 'P': return 0;
    case 'N': return 1;
    case 'B': return 2;
    case 'R': return 3;
    case 'Q': return 4;
    case 'K': return 5;
    case 'p': return 6;
    case 'n': return 7;
    case 'b': return 8;
    case 'r': return 9;
    case 'q': return 10;
    case 'k': return 11;
    default:  return -1;
  }
}