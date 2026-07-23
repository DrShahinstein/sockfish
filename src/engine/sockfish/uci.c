#include "uci.h"
#include "sockfish.h"
#include "evaluation.h"
#include "bitboard.h"
#include "move_helper.h"
#include "movegen.h"
#include "transposition_table.h"
#include "search.h"
#include "config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#define IS_TOKEN_END(ch) \
  ((ch) == '\n' || (ch) == '\r' || (ch) == ' ' || (ch) == '\t' || (ch) == '\0')

static const char *START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

/* Internal Helpers */
static void uci_parse_fen(const char *fen, SF_Context *ctx);
static Move uci_parse_move(SF_Context *ctx, const char *move_str);
static void print_best(Move best);
static void parse_go(const char *line, SF_Context *ctx);

/* Command Handlers */
static void handle_uci(const SF_Config *cfg);
static void handle_setoption(const char *line, SF_Config *cfg);
static void handle_ucinewgame(SF_Context *ctx);
static void handle_position(const char *line, SF_Context *ctx);
static void handle_go(const char *line, const SF_Context *uci_ctx, const SF_Config *cfg);

/* Async Logic */
static void *async_search_thread_main(void *arg);
static void async_search_reap(void);
static void async_search_start(const SF_Context *base_ctx, int threads);
static void async_search_request_stop(void);
static void async_search_shutdown(void);

static AsyncSearch async_search = {
  .thread_valid = false,
  .running      = false,
  .stop_flag    = false,
};

static inline void init_uci_config(SF_Config *cfg) {
  /* Default values for options */
  cfg->tt_size_mb = 16;
  cfg->threads    = 1;
}

static void apply_default_options(SF_Context *ctx, SF_Config *uci_cfg) {
  /* option name Hash */
  tt_init(uci_cfg->tt_size_mb);

  /* option name Threads */
  ctx->threads = uci_cfg->threads;

  /*- extras/3rd -*/
  ctx->allow_uci_info=true;
}

void uci_loop(void) {
  char line[8192];

  setbuf(stdin,  NULL);
  setbuf(stdout, NULL);

  SF_Config uci_config;
  init_uci_config(&uci_config);

  SF_Context uci_ctx;
  memset(&uci_ctx, 0, sizeof(SF_Context));
  apply_default_options(&uci_ctx, &uci_config);
  uci_parse_fen(START_FEN, &uci_ctx);

  while (fgets(line, sizeof(line), stdin)) {
    if (strncmp(line, "uci", 3) == 0 && IS_TOKEN_END(line[3])) {
      handle_uci(&uci_config);
    }
    else if (strncmp(line, "isready", 7) == 0 && IS_TOKEN_END(line[7])) {
      printf("readyok\n");
    }
    else if (strncmp(line, "setoption", 9) == 0 && IS_TOKEN_END(line[9])) {
      handle_setoption(line, &uci_config);
    }
    else if (strncmp(line, "ucinewgame", 10) == 0 && IS_TOKEN_END(line[10])) {
      handle_ucinewgame(&uci_ctx);
    }
    else if (strncmp(line, "position", 8) == 0 && IS_TOKEN_END(line[8])) {
      handle_position(line, &uci_ctx);
    }
    else if (strncmp(line, "go", 2) == 0 && IS_TOKEN_END(line[2])) {
      handle_go(line, &uci_ctx, &uci_config);
    }
    else if (strncmp(line, "d", 1) == 0 && IS_TOKEN_END(line[1])) {
      print_bitboard(uci_ctx.bitboard_set.occupied);
    }
    else if (strncmp(line, "stop", 4) == 0 && IS_TOKEN_END(line[4])) {
      async_search_request_stop();
    }
    else if (strncmp(line, "quit", 4) == 0 && IS_TOKEN_END(line[4])) {
      break;
    }
  }

  async_search_shutdown();
}

/* ==================== Command Handlers ==================== */

static void handle_uci(const SF_Config *cfg) {
  printf("id name Sockfish\n");
  printf("id author DrShahinstein\n");
  printf("option name Hash type spin default %d min 1 max 1024\n",   cfg->tt_size_mb);
  printf("option name Threads type spin default %d min 1 max 128\n", cfg->threads);
  printf("uciok\n");
}

static void handle_setoption(const char *line, SF_Config *cfg) {
  char *name_ptr = strstr(line, "name ");
  char *val_ptr  = strstr(line, "value ");

  if (!name_ptr || !val_ptr)
    return;

  name_ptr += 5;
  val_ptr  += 6;

  if (strncmp(name_ptr, "Hash", 4) == 0) {
    int new_hash = atoi(val_ptr);
    if (new_hash > 0 && new_hash != cfg->tt_size_mb) {
      cfg->tt_size_mb = new_hash;
      tt_free();
      tt_init(cfg->tt_size_mb);
    }
  }
  else if (strncmp(name_ptr, "Threads", 7) == 0) {
    int new_threads = atoi(val_ptr);
    if (new_threads > 0)
      cfg->threads = new_threads;
  }
}

static void handle_ucinewgame(SF_Context *ctx) {
  tt_clear();
  ctx->history_count = 0;
  memset(ctx->killer_moves,      0, sizeof(ctx->killer_moves));
  memset(ctx->history_heuristic, 0, sizeof(ctx->history_heuristic));
}

static void handle_position(const char *line, SF_Context *ctx) {
  if (strstr(line, "startpos")) {
    uci_parse_fen(START_FEN, ctx);
  } else if (strstr(line, "fen ")) {
    uci_parse_fen(strstr(line, "fen ") + 4, ctx);
  }

  char *moves_ptr = strstr(line, "moves ");
  if (!moves_ptr)
    return;

  moves_ptr += 6;
  char *move_str = strtok(moves_ptr, " \n");

  while (move_str != NULL) {
    Move m = uci_parse_move(ctx, move_str);
    if (m != 0) {
      MoveHistory hist;
      make_move(ctx, m, &hist);
    }
    move_str = strtok(NULL, " \n");
  }
}

static void handle_go(const char *line, const SF_Context *uci_ctx, const SF_Config *cfg) {
  SF_Context go_ctx = *uci_ctx;
  parse_go(line, &go_ctx);
  async_search_start(&go_ctx, cfg->threads);
}

static void parse_go(const char *line, SF_Context *ctx) {
  int wtime=0, btime=0, winc=0, binc=0, movetime=0;

  ctx->time_limit  = 0;
  ctx->depth_limit = 0;
  ctx->nodes_limit = 0;
  ctx->infinite    = false;

  if (strstr(line, "infinite"))
    ctx->infinite = true;

  if (strstr(line, "depth"))
    sscanf(strstr(line, "depth") + 6, "%d", &ctx->depth_limit);

  if (strstr(line, "nodes"))
    sscanf(strstr(line, "nodes") + 6, "%llu", (unsigned long long *)&ctx->nodes_limit);

  if (strstr(line, "movetime")) {
    sscanf(strstr(line, "movetime") + 9, "%d", &movetime);
    ctx->time_limit = movetime;
    return;
  }

  if (ctx->infinite || ctx->depth_limit || ctx->nodes_limit)
    return;

  if (strstr(line, "wtime")) sscanf(strstr(line, "wtime") + 5, "%d", &wtime);
  if (strstr(line, "btime")) sscanf(strstr(line, "btime") + 5, "%d", &btime);
  if (strstr(line, "winc"))  sscanf(strstr(line, "winc")  + 5, "%d", &winc);
  if (strstr(line, "binc"))  sscanf(strstr(line, "binc")  + 5, "%d", &binc);

  int time_left = (ctx->search_color == WHITE) ? wtime : btime;
  int inc       = (ctx->search_color == WHITE) ? winc  : binc;

  ctx->time_limit = (time_left / 30) + (inc / 2);
  if (ctx->time_limit < 50)
    ctx->time_limit = 50;
}

static void print_best(Move best) {
  char buf[6];
  move_to_uci_string(best, buf);
  printf("bestmove %s\n", buf);
}

static Move uci_parse_move(SF_Context *ctx, const char *move_str) {
  MoveList list = sf_generate_moves(ctx);

  for (int i = 0; i < list.count; ++i) {
    Move m = list.moves[i];
    char buf[6];
    move_to_uci_string(m, buf);

    if (strncmp(move_str, buf, strlen(buf)) == 0)
      return m;
  }

  return 0;
}

static void uci_parse_fen(const char *fen, SF_Context *ctx) {
  char board_char[8][8] = {0};
  Turn turn             = WHITE;
  uint8_t castling      = 0;
  Square ep_sq          = NO_ENPASSANT;

  char placement[256], active[2], castling_str[16], ep_str[3];
  int count = sscanf(fen, "%255s %1s %15s %2s", placement, active, castling_str, ep_str);

  if (count < 1) return;

  int row = 0, col = 0;
  for (const char *p = placement; *p && row < 8; ++p) {
    if (*p >= '1' && *p <= '8') {
      col += *p - '0';
    } else if (*p == '/') {
      row++;
      col = 0;
    } else {
      if (col < 8) board_char[row][col++] = *p;
    }
  }

  if (count >= 2)
    turn = (active[0] == 'w' || active[0] == 'W') ? WHITE : BLACK;

  if (count >= 3) {
    if (strcmp(castling_str, "-") != 0) {
      for (const char *p = castling_str; *p; ++p) {
        if (*p == 'K') castling |= CASTLE_WK;
        if (*p == 'Q') castling |= CASTLE_WQ;
        if (*p == 'k') castling |= CASTLE_BK;
        if (*p == 'q') castling |= CASTLE_BQ;
      }
    }
  } else castling = CASTLE_ALL;

  if (count >= 4 && strcmp(ep_str, "-") != 0) {
    int c = ep_str[0] - 'a';
    int r = 7 - (ep_str[1] - '1');
    ep_sq = rowcol_to_sq(r, c);
  }

  BitboardSet bbset = make_bitboards_from_charboard((const char (*)[8])board_char);

  ctx->bitboard_set    = bbset;
  ctx->search_color    = turn;
  ctx->castling_rights = castling;
  ctx->enpassant_sq    = ep_sq;
  ctx->halfmove_clock  = 0;
  ctx->history_count   = 0;

  sf_init_hash_key(ctx);
  sf_init_evaluation(ctx);
}

/* ==================== Async Logic ==================== */

static void *async_search_thread_main(void *arg) {
  (void)arg;

  Move best = sf_search(&async_search.ctx);
  print_best(best);
  atomic_store(&async_search.running, false);

  return NULL;
}

static void async_search_reap(void) {
  if (async_search.thread_valid && !atomic_load(&async_search.running)) {
    pthread_join(async_search.thread, NULL);
    async_search.thread_valid = false;
  }
}

static void async_search_start(const SF_Context *base_ctx, int threads) {
  if (atomic_load(&async_search.running))
    return;

  async_search_reap();

  async_search.ctx             = *base_ctx;
  async_search.ctx.threads     = threads;
  async_search.stop_flag       = false;
  async_search.ctx.should_stop = &async_search.stop_flag;

  atomic_store(&async_search.running, true);
  pthread_create(&async_search.thread, NULL, async_search_thread_main, NULL);
  async_search.thread_valid = true;
}

static void async_search_request_stop(void) {
  async_search.stop_flag = true;
}

static void async_search_shutdown(void) {
  async_search_request_stop();
  if (async_search.thread_valid) {
    pthread_join(async_search.thread, NULL);
    async_search.thread_valid = false;
  }
}

