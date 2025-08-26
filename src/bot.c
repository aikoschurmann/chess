#include "bot.h"
#include "engine.h"
#include "move_generation.h"
#include <stdlib.h>
#include <stdio.h>

static int moves_equal(const ChessMove *a, const ChessMove *b) {
    if (!a || !b) return 0;
    return a->start_tile == b->start_tile && a->end_tile == b->end_tile && a->move_type == b->move_type && a->promotion == b->promotion;
}

// Bot picks a move from the engine search; depth is fixed for now.
// Blocking bot: run iterative deepening for up to time_limit_ms and max_depth
void bot_select_move(ChessBoard *board, ChessMove *out_move) {
    // Prefer time-limited iterative deepening for stronger play
    int max_depth = 10; // allow deeper search
    int time_limit_ms = 5000; // think up to 5 seconds
    // For simplicity call engine_search_iterative synchronously
    engine_search_iterative(board, out_move, max_depth, NULL, NULL, time_limit_ms);

    // Validate returned move against generated legal moves; if not found, pick a fallback
    ChessMove moves[256]; int num_moves = 0;
    generate_moves_fast(board, moves, &num_moves);
    verify_king_safety(board, moves, &num_moves);

    int found = 0;
    for (int i = 0; i < num_moves; ++i) {
        if (moves_equal(&moves[i], out_move)) { found = 1; break; }
    }
    if (!found) {
        // fallback: choose first legal move if any
        if (num_moves > 0) {
            fprintf(stderr, "[bot] engine returned illegal move, falling back to first legal move\n");
            *out_move = moves[0];
        } else {
            // no legal moves - mark as invalid
            out_move->start_tile = -1;
            out_move->end_tile = -1;
        }
    }
}

void bot_play(ChessBoard *board) {
    ChessMove move;
    bot_select_move(board, &move);
    // If engine returned a move with invalid coordinates, do nothing.
    if (move.start_tile >= 0 && move.end_tile >= 0) {
        apply_move_simple(board, &move);
    }
}