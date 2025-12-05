#include "board_render.h"
#include "cursor.h"                /* get_mouse_pos() */
#include "sockfish/move_helper.h"  /* king_in_check() */
#include "engine.h"                /* make_bitboards_from_charboard() */
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

SDL_Texture *tex[128];
TTF_Font    *coord_font;
SDL_Texture *coord_tex_light['z' + 1];
SDL_Texture *coord_tex_dark ['z' + 1];

static void draw_capture_indicator(SDL_Renderer *renderer, int row, int col);
static void draw_square_highlight(SDL_Renderer *renderer, Square square, SDL_FColor color);
static void draw_arrow(SDL_Renderer *renderer, Square from_sq, Square to_sq, SDL_FColor color);

void render_board_init(SDL_Renderer *renderer) {
  const char *pieces = "rnbqkpRNBQKP";
  char path[256];

  for (const char *c = pieces; *c; ++c) {
    SDL_snprintf(path, sizeof path, PIECE_PATH, *c);
    SDL_Surface *surf = IMG_Load(path);

    if (!surf) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "IMG_Load failed: %s\n", SDL_GetError());
      continue;
    }

    tex[(int)*c] = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_DestroySurface(surf);

    if (!tex[(int)*c]) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "CreateTexture '%s' failed: %s\n", path, SDL_GetError());
    }
  }

  coord_font = TTF_OpenFont(DEJAVU_SANS_MONO, 12);
  if (!coord_font) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "TTF_OpenFont failed: %s\n", SDL_GetError());
    return;
  }

  SDL_Color dark_square_text_color  = {240, 217, 181, 255};
  SDL_Color light_square_text_color = {181, 136, 99,  255};

  const char *coords = "12345678abcdefgh";
  for (const char *c = coords; *c; ++c) {
    char text[] = {*c, '\0'};

    SDL_Surface *surf_light = TTF_RenderText_Blended(coord_font, text, 0, light_square_text_color);
    if (surf_light) {
      coord_tex_light[(int)*c] = SDL_CreateTextureFromSurface(renderer, surf_light);
      SDL_DestroySurface(surf_light);
    }

    SDL_Surface *surf_dark = TTF_RenderText_Blended(coord_font, text, 0, dark_square_text_color);
    if (surf_dark) {
      coord_tex_dark[(int)*c] = SDL_CreateTextureFromSurface(renderer, surf_dark);
      SDL_DestroySurface(surf_dark);
    }
  }
}

void render_board(SDL_Renderer *renderer, BoardState *board) {
  if (!renderer || !board) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "render_board: invalid parameters\n");
    return;
  }

  /*  Preperation  */
  float mx = 0, my = 0;
  int mouse_row = -1, mouse_col = -1;
  if (board->drag.active) {
    get_mouse_pos(&mx, &my);
    mouse_col = (int)(mx / SQ);
    mouse_row = (int)(my / SQ);
    if (mouse_row < 0 || mouse_row >= 8 || mouse_col < 0 || mouse_col >= 8) {
      mouse_row = -1;
      mouse_col = -1;
    }
  }

  /* Render Squares (light/dark) */
  for (int row = 0; row < 8; ++row) {
    for (int col = 0; col < 8; ++col) {
      SDL_FRect sq = {col * SQ, row * SQ, SQ, SQ};
      bool light = ((row + col) & 1) == 0;
      if (light) SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255);
      else       SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255);
      SDL_RenderFillRect(renderer, &sq);
    }
  }

  /* Render Manual Square Highlight (when pressed on right-click) */
  if (board->annotations.highlight_count > 0)
  {
    for (int i = 0; i < board->annotations.highlight_count; ++i) {
      Highlight *h = &board->annotations.highlights[i];
      draw_square_highlight(renderer, h->square, h->color);
    }
  }

  /* Render Coordination (a1,a2...) */
  for (int i = 0; i < 8; ++i) {
    char rank              = '8' - i;
    bool is_light          = ((i + 7) & 1) == 0;
    SDL_Texture *coord_tex = is_light ? coord_tex_light[(int)rank] : coord_tex_dark[(int)rank];

    if (coord_tex) {
      float w, h;
      SDL_GetTextureSize(coord_tex, &w, &h);
      SDL_FRect dst = {7 * SQ + SQ - w - 2.0f, i * SQ + 2.0f, w, h};
      SDL_RenderTexture(renderer, coord_tex, NULL, &dst);
    }

    char file = 'a' + i;
    is_light  = ((7 + i) & 1) == 0;
    coord_tex = is_light ? coord_tex_light[(int)file] : coord_tex_dark[(int)file];

    if (coord_tex) {
      float w, h;
      SDL_GetTextureSize(coord_tex, &w, &h);
      SDL_FRect dst = {i * SQ + 2.0f, 7 * SQ + SQ - h - 2.0f, w, h};
      SDL_RenderTexture(renderer, coord_tex, NULL, &dst);
    }
  }

  /* Highlight Last Move */
  if (board->undo_count > 0) {
    BoardMoveHistory *last_move = &board->history[board->undo_count - 1];
    SDL_FRect from_sq = {last_move->from_col * SQ, last_move->from_row * SQ, SQ, SQ};
    SDL_FRect to_sq   = {last_move->to_col   * SQ, last_move->to_row   * SQ, SQ, SQ};

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 180, 200, 70, 130);
    SDL_RenderFillRect(renderer, &from_sq);
    SDL_SetRenderDrawColor(renderer, 180, 200, 70, 130);
    SDL_RenderFillRect(renderer, &to_sq);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
  }

  /* Highlight Hover Over Valid Squares */
  bool hovering_valid_target = board->drag.active && mouse_row != -1 && mouse_col != -1 && board->selected_piece.active;
  if (hovering_valid_target) {
    Square from_sq       = rowcol_to_sq(board->selected_piece.row, board->selected_piece.col);
    bool is_valid_target = false;

    for (int i = 0; i < board->valid_moves.count; ++i) {
      Move move = board->valid_moves.moves[i];

      if (move_from(move) == from_sq) {
        Square to_sq = move_to(move);
        int to_row   = square_to_row(to_sq);
        int to_col   = square_to_col(to_sq);

        if (to_row == mouse_row && to_col == mouse_col) {
          is_valid_target = true;
          break;
        }
      }
    }

    if (is_valid_target) {
      SDL_FRect target_rect = {mouse_col * SQ, mouse_row * SQ, SQ, SQ};
      SDL_SetRenderDrawColor(renderer, 20, 20, 20, 70);
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
      SDL_RenderFillRect(renderer, &target_rect);
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
  }

  /* Render Pieces Excluding Drag */
  for (int row = 0; row < 8; ++row) {
    for (int col = 0; col < 8; ++col) {
      bool is_dragged_piece = board->drag.active && row == board->drag.from_row && col == board->drag.from_col;

      if (is_dragged_piece)
        continue;

      char pc = board->board[row][col];
      if (pc && tex[(int)pc]) {
        SDL_FRect dst = {col * SQ, row * SQ, SQ, SQ};
        SDL_RenderTexture(renderer, tex[(int)pc], NULL, &dst);
      }
    }
  }

  /* Render Dragging Piece */
  if (board->drag.active) {
    char pc = board->board[board->drag.from_row][board->drag.from_col];
    if (pc && tex[(int)pc]) {
      SDL_FRect dst = {mx - SQ / 2.0f, my - SQ / 2.0f, SQ, SQ};
      SDL_RenderTexture(renderer, tex[(int)pc], NULL, &dst);
    }
  }

  /* Render Pawn Promote Menu */
  if (board->promo.active) {
    for (int i = 0; i < 4; ++i) {
      float menu_x = board->promo.col * SQ;
      float menu_y = board->turn == WHITE ? (i*SQ) : ((7-i) * SQ);

      SDL_FRect dst = {menu_x, menu_y, SQ, SQ};
      SDL_SetRenderDrawColor(renderer, 169, 169, 169, 255);
      SDL_RenderFillRect(renderer, &dst);

      char pc = board->promo.choices[i];
      if (pc && tex[(int)pc]) {
        SDL_RenderTexture(renderer, tex[(int)pc], NULL, &dst);
      }
    }
  }

  /* Render Valid Moves For The Selected Piece */
  if (board->selected_piece.active) {
    char pc = board->board[board->selected_piece.row][board->selected_piece.col];

    if (pc && tex[(int)pc]) {
      SelectedPiece s    = board->selected_piece;
      Square selected_sq = rowcol_to_sq(s.row, s.col);

      for (int i = 0; i < board->valid_moves.count; ++i) {
        Move mv = board->valid_moves.moves[i];

        if (move_from(mv) != selected_sq) continue;

        Square to = move_to(mv);
        int row   = square_to_row(to);
        int col   = square_to_col(to);

        bool is_drag_hovered_square = board->drag.active && mouse_row != -1 && mouse_col != -1 && row == mouse_row && col == mouse_col;
        if (is_drag_hovered_square) continue;

        bool is_capture = board->board[row][col] != 0;
        float centerX   = col * SQ + (SQ / 2.0f);
        float centerY   = row * SQ + (SQ / 2.0f);

        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 80);

        /* Dot */
        if (!is_capture) {
          float radius  = 8.0f;
          float outer_r = radius + 2.5f;
          int r_out     = (int)SDL_ceilf(outer_r);

          for (int dy = -r_out; dy <= r_out; ++dy) {
            float dyf      = (float)dy;
            float dx       = SDL_sqrtf(SDL_max(0.0f, outer_r * outer_r - dyf * dyf));
            SDL_FRect span = {centerX - dx, centerY + dyf - 0.5f, dx * 2.0f, 1.0f};

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(renderer, &span);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
          }
        }

        /* Triangles */
        else {
          draw_capture_indicator(renderer, row, col);
        }
      }
    }
  }

  /* Attention Over Checks */
  if (board->king.in_check) {
    SDL_SetRenderDrawColor(renderer, 255, 20, 20, 200);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    int border_thickness = 2;
    for (int i = 0; i < border_thickness; ++i) {
      SDL_FRect border_rect = {(float)(board->king.col * SQ + i),
                               (float)(board->king.row * SQ + i), (float)(SQ - i * 2),
                               (float)(SQ - i * 2)};
      SDL_RenderRect(renderer, &border_rect);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
  }

  /* Render Arrows (while moving right-click) */
  if (board->annotations.arrow_count > 0)
  {
    for (int i = 0; i < board->annotations.arrow_count; ++i) {
      Arrow *a = &board->annotations.arrows[i];
      draw_arrow(renderer, a->from, a->to, a->color);
    }
  }
}

void render_board_cleanup(void) {
  for (int i = 0; i < 128; ++i) {
    if (tex[i]) {
      SDL_DestroyTexture(tex[i]);
      tex[i] = NULL;
    }
  }

  for (int i = 0; i < 'z' + 1; ++i) {
    if (coord_tex_light[i]) {
      SDL_DestroyTexture(coord_tex_light[i]);
      coord_tex_light[i] = NULL;
    }
    if (coord_tex_dark[i]) {
      SDL_DestroyTexture(coord_tex_dark[i]);
      coord_tex_dark[i] = NULL;
    }
  }

  if (coord_font) {
    TTF_CloseFont(coord_font);
    coord_font = NULL;
  }
}

static void draw_capture_indicator(SDL_Renderer *renderer, int row, int col) {
    float triangle_size = 15.0f;
    SDL_FColor c        = {20.0f/255.0f, 20.0f/255.0f, 20.0f/255.0f, 80.0f/255.0f};
    
    SDL_Vertex vertices[12]; // 4 triangles * 3 vertices each
    
    float x = col * SQ;
    float y = row * SQ;

    // colors
    for (int i = 0; i < 12; i++) {
      vertices[i].color = c;
    }

    // top-left triangle
    vertices[0].position = (SDL_FPoint){x, y};
    vertices[1].position = (SDL_FPoint){x + triangle_size, y};
    vertices[2].position = (SDL_FPoint){x, y + triangle_size};
    // top-right triangle
    vertices[3].position = (SDL_FPoint){x + SQ, y};
    vertices[4].position = (SDL_FPoint){x + SQ - triangle_size, y};
    vertices[5].position = (SDL_FPoint){x + SQ, y + triangle_size};
    // bottom-right triangle
    vertices[6].position = (SDL_FPoint){x + SQ, y + SQ};
    vertices[7].position = (SDL_FPoint){x + SQ - triangle_size, y + SQ};
    vertices[8].position = (SDL_FPoint){x + SQ, y + SQ - triangle_size};
    // bottom-left triangle
    vertices[9].position  = (SDL_FPoint){x, y + SQ};
    vertices[10].position = (SDL_FPoint){x + triangle_size, y + SQ};
    vertices[11].position = (SDL_FPoint){x, y + SQ - triangle_size};
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderGeometry(renderer, NULL, vertices, 12, NULL, 0);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

static void draw_square_highlight(SDL_Renderer *renderer, Square square, SDL_FColor color) {
  int row = square_to_row(square);
  int col = square_to_col(square);

  SDL_FRect rect = {col * SQ, row * SQ, SQ, SQ};

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  SDL_SetRenderDrawColor(renderer, (Uint8)(color.r * 255), (Uint8)(color.g * 255), (Uint8)(color.b * 255), (Uint8)(color.a * 255));
  SDL_RenderFillRect(renderer, &rect);

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

static void draw_arrow(SDL_Renderer *renderer, Square from_sq, Square to_sq, SDL_FColor color) {
  int from_row = square_to_row(from_sq);
  int from_col = square_to_col(from_sq);
  int to_row   = square_to_row(to_sq);
  int to_col   = square_to_col(to_sq);

  float from_x = from_col * SQ + SQ / 2.0f;
  float from_y = from_row * SQ + SQ / 2.0f;
  float to_x   = to_col   * SQ + SQ / 2.0f;
  float to_y   = to_row   * SQ + SQ / 2.0f;

  float dx  = to_x - from_x;
  float dy  = to_y - from_y;
  float len = SDL_sqrtf(dx * dx + dy * dy);
  
  if (len < 0.1f) return;
  
  dx /= len;
  dy /= len;

  float arrow_width       = 10.0f;
  float arrow_head_length = 30.0f;
  float arrow_head_width  = 40.0f;

  float shaft_end_x = to_x - dx * arrow_head_length;
  float shaft_end_y = to_y - dy * arrow_head_length;

  float perp_x = -dy;
  float perp_y = dx;

  SDL_Vertex shaft[6];
  for (int i = 0; i < 6; ++i) {
    shaft[i].color = color;
  }

  shaft[0].position = (SDL_FPoint){from_x + perp_x * arrow_width / 2, from_y + perp_y * arrow_width / 2};
  shaft[1].position = (SDL_FPoint){from_x - perp_x * arrow_width / 2, from_y - perp_y * arrow_width / 2};
  shaft[2].position = (SDL_FPoint){shaft_end_x + perp_x * arrow_width / 2, shaft_end_y + perp_y * arrow_width / 2};
  shaft[3].position = (SDL_FPoint){shaft_end_x + perp_x * arrow_width / 2, shaft_end_y + perp_y * arrow_width / 2};
  shaft[4].position = (SDL_FPoint){from_x - perp_x * arrow_width / 2, from_y - perp_y * arrow_width / 2};
  shaft[5].position = (SDL_FPoint){shaft_end_x - perp_x * arrow_width / 2, shaft_end_y - perp_y * arrow_width / 2};

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_RenderGeometry(renderer, NULL, shaft, 6, NULL, 0);

  SDL_Vertex head[3];
  for (int i = 0; i < 3; i++) {
    head[i].color = color;
  }

  head[0].position = (SDL_FPoint){to_x, to_y};
  head[1].position = (SDL_FPoint){shaft_end_x + perp_x * arrow_head_width / 2, shaft_end_y + perp_y * arrow_head_width / 2};
  head[2].position = (SDL_FPoint){shaft_end_x - perp_x * arrow_head_width / 2, shaft_end_y - perp_y * arrow_head_width / 2};

  SDL_RenderGeometry(renderer, NULL, head, 3, NULL, 0);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}