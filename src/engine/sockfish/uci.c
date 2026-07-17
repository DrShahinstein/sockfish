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

#define IS_TOKEN_END(ch) \
  ((ch) == '\n' || (ch) == '\r' || (ch) == ' ' || (ch) == '\t' || (ch) == '\0')

static const char *START_FEN="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static Move uci_parse_move(SF_Context *ctx, const char *move_str);
static void uci_parse_fen(const char *fen, SF_Context *ctx);

static inline void init_uci_config(SF_Config *cfg) {
  /* Default values for options */
  cfg->tt_size_mb = 16;
  cfg->threads    = 1;
}

static void apply_default_options(SF_Context *ctx, SF_Config *uci_cfg) {
  /* option name Hash */
  int megabytes = uci_cfg->tt_size_mb;
  tt_init(megabytes);

  /* option name Threads */
  ctx->threads = uci_cfg->threads;

  /*- extras/3rd -*/
  ctx->allow_uci_info=true;
}

void uci_loop(void) {
  char line[2048];

  setbuf(stdin,  NULL);
  setbuf(stdout, NULL);

  SF_Config uci_config;
  init_uci_config(&uci_config);

  SF_Context uci_ctx;
  apply_default_options(&uci_ctx, &uci_config);
  uci_parse_fen(START_FEN, &uci_ctx);

  while (fgets(line, sizeof(line), stdin)) {
    if (strncmp(line, "uci", 3) == 0 && IS_TOKEN_END(line[3])) {
      printf("id name Sockfish\n");
      printf("id author DrShahinstein\n");
      printf("option name Hash type spin default %d min 1 max 1024\n",   uci_config.tt_size_mb);
      printf("option name Threads type spin default %d min 1 max 128\n", uci_config.threads);
      printf("uciok\n");
    }
    
    else if (strncmp(line, "isready", 7) == 0) {
      printf("readyok\n");
    }

    else if (strncmp(line, "setoption", 9) == 0) {
      char *name_ptr = strstr(line, "name ");
      char *val_ptr  = strstr(line, "value ");
      
      if (name_ptr && val_ptr) {
        name_ptr += 5;
        val_ptr  += 6;
        
        if (strncmp(name_ptr, "Hash", 4) == 0) {
          int new_hash = atoi(val_ptr);
          if (new_hash > 0 && new_hash != uci_config.tt_size_mb) {
            uci_config.tt_size_mb = new_hash;
            tt_free();
            tt_init(uci_config.tt_size_mb);
          }
        }

        else if (strncmp(name_ptr, "Threads", 7) == 0) {
          int new_threads = atoi(val_ptr);
          if (new_threads > 0) {
            uci_config.threads = new_threads;
          }
        }
      }
    }
    
    else if (strncmp(line, "ucinewgame", 10) == 0) {
      tt_clear();
      uci_ctx.history_count = 0;
      memset(uci_ctx.killer_moves, 0, sizeof(uci_ctx.killer_moves));
    }
    
    else if (strncmp(line, "position", 8) == 0) {
      if (strstr(line, "startpos")) {
        uci_parse_fen(START_FEN, &uci_ctx);
      }

      else if (strstr(line, "fen ")) {
        char *fen_ptr = strstr(line, "fen ") + 4;
        uci_parse_fen(fen_ptr, &uci_ctx);
      }
      
      char *moves_ptr = strstr(line, "moves ");
      if (moves_ptr != NULL) {
        moves_ptr += 6; 
        char *move_str = strtok(moves_ptr, " \n");
        
        while (move_str != NULL) {
          Move m = uci_parse_move(&uci_ctx, move_str);
          if (m != 0) {
            MoveHistory hist;
            make_move(&uci_ctx, m, &hist);
          }
          move_str = strtok(NULL, " \n");
        }
      }
    }
    
    else if (strncmp(line, "go", 2) == 0) {
      int wtime = 0, btime = 0, winc = 0, binc = 0, movetime = 0;
      
      if (strstr(line, "movetime")) {
        sscanf(strstr(line, "movetime") + 8, "%d", &movetime);
        uci_ctx.time_limit = movetime;
      } else {
        if (strstr(line, "wtime")) sscanf(strstr(line, "wtime") + 5, "%d", &wtime);
        if (strstr(line, "btime")) sscanf(strstr(line, "btime") + 5, "%d", &btime);
        if (strstr(line, "winc"))  sscanf(strstr(line, "winc")  + 5, "%d", &winc);
        if (strstr(line, "binc"))  sscanf(strstr(line, "binc")  + 5, "%d", &binc);
        
        int time_left = (uci_ctx.search_color == WHITE) ? wtime : btime;
        int inc       = (uci_ctx.search_color == WHITE) ? winc  : binc;
        
        uci_ctx.time_limit = (time_left / 40) + (inc / 2);
        if (uci_ctx.time_limit < 100) uci_ctx.time_limit = 100;
      }

      uci_ctx.threads = uci_config.threads;
      
      Move best = sf_search(&uci_ctx);
      
      char from_alg[3], to_alg[3];
      sq_to_alg(move_from(best), from_alg);
      sq_to_alg(move_to(best),   to_alg);
      
      if (move_type(best) == MOVE_PROMOTION) {
        char promo = 'q';

        switch (move_promotion(best)) {
          case PROMOTE_ROOK:   promo = 'r'; break;
          case PROMOTE_BISHOP: promo = 'b'; break;
          case PROMOTE_KNIGHT: promo = 'n'; break;
          default:             promo = 'q'; break;
        }

        printf("bestmove %s%s%c\n", from_alg, to_alg, promo);
      } else {
        printf("bestmove %s%s\n", from_alg, to_alg);
      }
    }

    else if (strncmp(line, "d", 1) == 0 && line[1] == '\n') {
      print_bitboard(uci_ctx.bitboard_set.occupied);
    }
    
    else if (strncmp(line, "quit", 4) == 0) {
      break;
    }
  }
}

static Move uci_parse_move(SF_Context *ctx, const char *move_str) {
  MoveList list = sf_generate_moves(ctx);
  
  for (int i = 0; i < list.count; ++i) {
    Move m = list.moves[i];
    char buf[6];
    
    char from_alg[3], to_alg[3];
    sq_to_alg(move_from(m), from_alg);
    sq_to_alg(move_to(m), to_alg);
    
    if (move_type(m) == MOVE_PROMOTION) {
      char promo = 'q';
      switch (move_promotion(m)) {
        case PROMOTE_ROOK:   promo = 'r'; break;
        case PROMOTE_BISHOP: promo = 'b'; break;
        case PROMOTE_KNIGHT: promo = 'n'; break;
        default:             promo = 'q'; break;
      }
      snprintf(buf, sizeof(buf), "%s%s%c", from_alg, to_alg, promo);
    } else {
      snprintf(buf, sizeof(buf), "%s%s", from_alg, to_alg);
    }
    
    if (strncmp(move_str, buf, strlen(buf)) == 0) {
      return m;
    }
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
      for (const char *p = castling_str; *p; p++) {
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

