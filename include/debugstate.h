#ifndef DEBUGSTATE_H
#define DEBUGSTATE_H

#include "chess_bitboard.h"
#include "chess_gui.h"

typedef struct {
    Piece selected_piece;          // The currently selected piece type (e.g., PAWN, ROOK, etc.)
    int draw_white_bitboard_mask;  // 1 for white's turn, 0 for black's turn
    int should_draw_bitboards;     // 1 to draw bitboards, 0 to not draw them
} DebugState;

void draw_selected_bitboard(const DebugState *debugstate, ChessBoard *board, GameUIState *ui_state);
#endif // DEBUGSTATE_H
