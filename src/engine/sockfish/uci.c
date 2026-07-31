#include "uci.h"
#include "sockfish.h"
#include "evaluation.h"
#include "fen.h"
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

typedef struct {
  const char *text;
  size_t length;
} UciToken;

/* Internal Helpers */
static bool next_token(const char **cursor, UciToken *token);
static bool token_equals(UciToken token, const char *expected);
static bool uci_parse_fen(const char *fen, SF_Context *ctx);
static bool uci_parse_fen_tokens(const UciToken fields[6], SF_Context *ctx);
static Move uci_parse_move(SF_Context *ctx, UciToken move_token);
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
  config_init_default(cfg);
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

  if (!uci_parse_fen(START_FEN, &uci_ctx))
    return;

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
  printf("option name Hash type spin default %d min %d max %d\n", cfg->tt_size_mb, SF_TT_SIZE_MB_MIN, SF_TT_SIZE_MB_MAX);
  printf("option name Threads type spin default %d min %d max %d\n", cfg->threads, SF_THREADS_MIN, SF_THREADS_MAX);
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
    int new_hash;
    if (parse_bounded_int(val_ptr, SF_TT_SIZE_MB_MIN, SF_TT_SIZE_MB_MAX, &new_hash) && new_hash != cfg->tt_size_mb) {
      async_search_shutdown();
      cfg->tt_size_mb = new_hash;
      tt_free();
      tt_init(cfg->tt_size_mb);
    }
  }
  else if (strncmp(name_ptr, "Threads", 7) == 0) {
    int new_threads;
    if (parse_bounded_int(val_ptr, SF_THREADS_MIN, SF_THREADS_MAX, &new_threads))
      cfg->threads = new_threads;
  }
}

static void handle_ucinewgame(SF_Context *ctx) {
  async_search_shutdown();
  tt_clear();
  ctx->history_count                  = 1;
  ctx->history_head                   = 0;
  ctx->pos_history[ctx->history_head] = ctx->hash_key;
  ctx->in_null_search                 = false;
  ctx->nmp_min_ply                    = 0;
  memset(ctx->killer_moves,      0, sizeof(ctx->killer_moves));
  memset(ctx->history_heuristic, 0, sizeof(ctx->history_heuristic));
}

static void handle_position(const char *line, SF_Context *ctx) {
  const char *cursor = line;
  UciToken token;

  if (!next_token(&cursor, &token) || !token_equals(token, "position"))
    return;

  SF_Context candidate = *ctx;

  if (!next_token(&cursor, &token))
    return;

  if (token_equals(token, "startpos")) {
    if (!uci_parse_fen(START_FEN, &candidate))
      return;
  } else if (token_equals(token, "fen")) {
    UciToken fen_fields[6];
    for (int i = 0; i < 6; ++i) {
      if (!next_token(&cursor, &fen_fields[i]))
        return;
    }

    if (!uci_parse_fen_tokens(fen_fields, &candidate))
      return;
  } else {
    return;
  }

  if (!next_token(&cursor, &token)) {
    *ctx = candidate;
    return;
  }

  if (!token_equals(token, "moves"))
    return;

  while (next_token(&cursor, &token)) {
    Move move = uci_parse_move(&candidate, token);
    if (move == MOVE_NONE)
      return;

    MoveHistory history;
    make_move(&candidate, move, &history);
  }

  *ctx = candidate;
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
  if (best == MOVE_NONE) {
    printf("bestmove 0000\n");
    return;
  }

  char buf[6];
  move_to_uci_string(best, buf);
  printf("bestmove %s\n", buf);
}

static Move uci_parse_move(SF_Context *ctx, UciToken move_token) {
  MoveList list = sf_generate_moves(ctx);

  for (int i = 0; i < list.count; ++i) {
    Move m = list.moves[i];
    char buf[6];
    move_to_uci_string(m, buf);

    size_t move_length = strlen(buf);
    if (move_token.length == move_length && memcmp(move_token.text, buf, move_length) == 0) {
      return m;
    }
  }

  return MOVE_NONE;
}

static bool next_token(const char **cursor, UciToken *token) {
  const char *start = *cursor;
  while (*start != '\0' && IS_TOKEN_END(*start))
    ++start;

  if (*start == '\0') {
    *cursor = start;
    return false;
  }

  const char *end = start;
  while (!IS_TOKEN_END(*end))
    ++end;

  token->text   = start;
  token->length = (size_t)(end - start);
  *cursor       = end;
  return true;
}

static bool token_equals(UciToken token, const char *expected) {
  size_t expected_length = strlen(expected);
  return token.length == expected_length && memcmp(token.text, expected, expected_length) == 0;
}

static bool uci_parse_fen(const char *fen, SF_Context *ctx) {
  SF_Fen parsed;
  if (!sf_parse_fen(fen, &parsed))
    return false;

  BitboardSet bbset    = make_bitboards_from_charboard((const char (*)[8])parsed.board);
  SF_Context candidate = *ctx;

  candidate.bitboard_set    = bbset;
  candidate.search_color    = parsed.turn;
  candidate.castling_rights = parsed.castling_rights;
  candidate.enpassant_sq    = parsed.enpassant_sq;
  candidate.halfmove_clock  = parsed.halfmove_clock;
  candidate.history_count   = 1;
  candidate.history_head    = 0;
  candidate.in_null_search  = false;
  candidate.nmp_min_ply     = 0;

  sf_init_hash_key(&candidate);
  sf_init_evaluation(&candidate);

  candidate.pos_history[candidate.history_head] = candidate.hash_key;
  *ctx = candidate;

  return true;
}

static bool uci_parse_fen_tokens(const UciToken fields[6], SF_Context *ctx) {
  char fen[SF_FEN_MAX_LENGTH];
  size_t offset = 0;

  for (int i = 0; i < 6; ++i) {
    if (i != 0) {
      if (offset + 1U >= sizeof(fen))
        return false;
      fen[offset++] = ' ';
    }

    if (fields[i].length >= sizeof(fen) - offset)
      return false;

    memcpy(&fen[offset], fields[i].text, fields[i].length);
    offset += fields[i].length;
  }

  fen[offset] = '\0';

  return uci_parse_fen(fen, ctx);
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
  async_search.ctx.should_stop = &async_search.stop_flag;
  atomic_store_explicit(&async_search.stop_flag, false, memory_order_relaxed);
  atomic_store(&async_search.running, true);

  int result = pthread_create(&async_search.thread, NULL, async_search_thread_main, NULL);
  if (result != 0) {
    atomic_store(&async_search.running, false);
    async_search.thread_valid = false;
    print_best(MOVE_NONE);
    return;
  }

  async_search.thread_valid = true;
}

static void async_search_request_stop(void) {
  atomic_store_explicit(&async_search.stop_flag, true, memory_order_relaxed);
}

static void async_search_shutdown(void) {
  async_search_request_stop();
  if (async_search.thread_valid) {
    pthread_join(async_search.thread, NULL);
    async_search.thread_valid = false;
  }
}
