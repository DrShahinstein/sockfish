#include "app.h"
#include "sockfish/transposition_table.h"

int main(int argc, char *argv[]) {
  init_zobrist_keys();    // init zobrist hashing
  init_attack_tables();   // init precomputed attack tables for sockfish's move generation logic
  init_magic_bitboards(); // init magic bitboards for sliding pieces in move generation logic

  bool ran_with_uci_param = argc > 1 && strcmp(argv[1], "uci") == 0;

  if (ran_with_uci_param) {
    uci();
  }

  else {
    sdl();
  }

  cleanup_magic_bitboards();
  tt_free();

  return 0;
}

