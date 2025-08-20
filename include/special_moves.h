#pragma once

#include "board.h"

void update_castling_rights(BoardState *board, char moving_piece, Move *move);
bool is_castling_move(BoardState *board, Move *move);
void perform_castling(BoardState *board, Move *move);
bool is_en_passant_capture(BoardState *board, Move *move);
void update_enpassant_rights(BoardState *board, char moving_piece);