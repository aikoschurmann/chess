#include "debugstate.h"

void draw_selected_bitboard(const DebugState *debugstate, ChessBoard *board) {
    if (debugstate->draw_white_bitboard_mask && debugstate->should_draw_bitboards) {
        draw_bitboard_mask(board->pieces[WHITE][debugstate->selected_piece], 255, 0, 0, 100); // Red for white
    } else if (!debugstate->draw_white_bitboard_mask && debugstate->should_draw_bitboards) {
        draw_bitboard_mask(board->pieces[BLACK][debugstate->selected_piece], 0, 0, 255, 100); // Blue for black
    }
}