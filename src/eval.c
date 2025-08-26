#include "eval.h"

int piece_value(int piece_type) {
    switch (piece_type) {
        case PAWN: return 100;
        case KNIGHT: return 320;
        case BISHOP: return 330;
        case ROOK: return 500;
        case QUEEN: return 900;
        case KING: return 20000;
        default: return 0;
    }
}

int evaluate_material(ChessBoard *board) {
    int score = 0;
    for (int color = 0; color < 2; ++color) {
        int sign = (color == board->current_turn) ? 1 : -1;
        for (int p = 0; p < 6; ++p) {
            Bitboard bb = board->pieces[color][p];
            while (bb) {
                int idx = __builtin_ctzll(bb);
                score += sign * piece_value(p);
                bb &= bb - 1;
            }
        }
    }
    return score;
}
