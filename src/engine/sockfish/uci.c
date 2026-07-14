#include "uci.h"
#include "sockfish.h"
#include "bitboard.h"
#include "move_helper.h"
#include "movegen.h"
#include "transposition_table.h"
#include "search.h"
#include <string.h>
#include <stdio.h>

static Move uci_parse_move(SF_Context *ctx, const char *move_str);
static void uci_parse_fen(const char *fen, SF_Context *ctx);

static const char START_BOARD[8][8] = {
  {'r','n','b','q','k','b','n','r'},
  {'p','p','p','p','p','p','p','p'},
  { 0,  0,  0,  0,  0,  0,  0,  0 },
  { 0,  0,  0,  0,  0,  0,  0,  0 },
  { 0,  0,  0,  0,  0,  0,  0,  0 },
  { 0,  0,  0,  0,  0,  0,  0,  0 },
  {'P','P','P','P','P','P','P','P'},
  {'R','N','B','Q','K','B','N','R'}
};

void uci_loop(void) {
  char line[2048];

  setbuf(stdin,  NULL);
  setbuf(stdout, NULL);

  BitboardSet bbset  = make_bitboards_from_charboard(START_BOARD);
  SF_Context uci_ctx = create_sf_ctx(&bbset, WHITE, CASTLE_ALL, NO_ENPASSANT);

  while (fgets(line, sizeof(line), stdin)) {
    if (strncmp(line, "uci", 3) == 0 && line[3] != 'o') {
      printf("id name Sockfish\n");
      printf("id author DrShahinstein\n");
      printf("uciok\n");
    }
    
    else if (strncmp(line, "isready", 7) == 0) {
      printf("readyok\n");
    }
    
    else if (strncmp(line, "ucinewgame", 10) == 0) {
      tt_clear();
      uci_ctx.history_count = 0;
    }
    
    else if (strncmp(line, "position", 8) == 0) {
      if (strstr(line, "startpos")) {
        bbset   = make_bitboards_from_charboard(START_BOARD);
        uci_ctx = create_sf_ctx(&bbset, WHITE, CASTLE_ALL, NO_ENPASSANT);
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
      int wtime = 0, btime = 0, movetime = 0;
      
      if (strstr(line, "movetime")) {
        sscanf(strstr(line, "movetime") + 8, "%d", &movetime);
        uci_ctx.time_limit = movetime;
      } else {
        if (strstr(line, "wtime")) sscanf(strstr(line, "wtime") + 5, "%d", &wtime);
        if (strstr(line, "btime")) sscanf(strstr(line, "btime") + 5, "%d", &btime);
        
        int time_left      = (uci_ctx.search_color == WHITE) ? wtime : btime;
        uci_ctx.time_limit = time_left / 30;
        if (uci_ctx.time_limit < 100) uci_ctx.time_limit = 100;
      }
      
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
    
    else if (strncmp(line, "quit", 4) == 0) {
      break;
    }
  }
}

static Move uci_parse_move(SF_Context *ctx, const char *move_str) {
  MoveList list = sf_generate_moves(ctx);
  
  for (int i = 0; i < list.count; i++) {
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
  *ctx = create_sf_ctx(&bbset, turn, castling, ep_sq);
}

