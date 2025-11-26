#include "board.h"

inline void clear_annotations(Annotations *anns) {
  if (!anns) return;

  anns->highlight_count = 0;
  anns->arrow_count     = 0;
  anns->drawing_arrow   = false;
}

inline void start_drawing_arrow(Annotations *anns, Square start) {
  if (!anns) return;

  anns->drawing_arrow = true;
  anns->arrow_start   = start;
}

inline void cancel_drawing_arrow(Annotations *anns) {
  if (!anns) return;

  anns->drawing_arrow = false;
}

inline bool is_drawing_arrow(const Annotations *anns) {
  return anns && anns->drawing_arrow;
}

inline Square get_arrow_start(const Annotations *anns) {
  return anns ? anns->arrow_start : A1;
}

inline int get_arrow_count(const Annotations *anns) {
  return anns ? anns->arrow_count : 0;
}

inline int get_highlight_count(const Annotations *anns) {
  return anns ? anns->highlight_count : 0;
}

bool has_arrow(Annotations *anns, Square from, Square to) {
  if (!anns) return false;

  for (int i = 0; i < anns->arrow_count; ++i)
    if (anns->arrows[i].from == from && anns->arrows[i].to == to)
      return true;

  return false;
}

bool add_arrow(Annotations *anns, Square from, Square to, SDL_FColor color) {
  if (!anns || anns->arrow_count >= MAX_ARROWS || from == to)
    return false;

  for (int i = 0; i < anns->arrow_count; ++i) {
    if (anns->arrows[i].from == from && anns->arrows[i].to == to) {
      anns->arrows[i].color = color;
      return true;
    }
  }

  anns->arrows[anns->arrow_count] = (Arrow){from, to, color};
  anns->arrow_count++;

  return true;
}

bool remove_arrow(Annotations *anns, Square from, Square to) {
  if (!anns) return false;

  for (int i = 0; i < anns->arrow_count; ++i) {
    if (anns->arrows[i].from == from && anns->arrows[i].to == to) {
      for (int j = i; j < anns->arrow_count - 1; ++j)
        anns->arrows[j] = anns->arrows[j + 1];

      anns->arrow_count--;
      return true;
    }
  }

  return false;
}

bool add_highlight(Annotations *anns, Square square, SDL_FColor color) {
  if (!anns || anns->highlight_count >= MAX_HIGHLIGHTS)
    return false;

  for (int i = 0; i < anns->highlight_count; ++i) {
    if (anns->highlights[i].square == square) {
      anns->highlights[i].color = color;
      return true;
    }
  }

  anns->highlights[anns->highlight_count] = (Highlight){square, color};
  anns->highlight_count++;

  return true;
}

bool remove_highlight(Annotations *anns, Square square) {
  if (!anns) return false;

  for (int i = 0; i < anns->highlight_count; ++i) {
    if (anns->highlights[i].square == square) {
      for (int j = i; j < anns->highlight_count - 1; ++j)
        anns->highlights[j] = anns->highlights[j + 1];

      anns->highlight_count--;
      return true;
    }
  }

  return false;
}

bool has_highlight(Annotations *anns, Square square) {
  if (!anns) return false;

  for (int i = 0; i < anns->highlight_count; ++i)
    if (anns->highlights[i].square == square)
      return true;
      
  return false;
}