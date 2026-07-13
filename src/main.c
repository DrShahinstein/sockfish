#include "app.h"
#include "sockfish/transposition_table.h"

int main(int argc, char *argv[]) {
  init_attack_tables();   // init precomputed attack tables for sockfish's move generation logic
  init_magic_bitboards(); // init magic bitboards for sliding pieces in move generation logic
  tt_init(32);            // init 32MB transposition table (≈2.000.000 positions)

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
