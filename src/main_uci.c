#include "app.h"
#include "sockfish/transposition_table.h"

int main(void) {
  init_zobrist_keys();    // init zobrist hashing
  init_attack_tables();   // init precomputed attack tables for sockfish's move generation logic
  init_magic_bitboards(); // init magic bitboards for sliding pieces in move generation logic

  uci();

  cleanup_magic_bitboards();
  tt_free();

  return 0;
}

