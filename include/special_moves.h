#pragma once

#include "board.h"

void update_castling_rights(BoardState *board, char moving_piece, MoveRC *move);
bool is_castling_move(BoardState *board, MoveRC *move);
void perform_castling(BoardState *board, MoveRC *move);
bool is_en_passant_capture(BoardState *board, MoveRC *move);
void update_enpassant_rights(BoardState *board, char moving_piece);