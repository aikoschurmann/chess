#include "engine.h"
#include "search.h"
#include "eval.h"
#include "move_generation.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

void engine_init(void) {
    // placeholder for any engine-wide initialization
}

// Simple perft using generate_moves_fast and apply_move_simple semantics from engine
static unsigned long long perft_inner(ChessBoard *board, int depth) {
    if (depth == 0) return 1ULL;

    ChessMove moves[256];
    int num_moves = 0;
    generate_moves_fast(board, moves, &num_moves);
    unsigned long long nodes = 0ULL;

    for (int i = 0; i < num_moves; ++i) {
        // make move
        ChessBoard copy = *board;
        apply_move_simple(&copy, &moves[i]);
        nodes += perft_inner(&copy, depth - 1);
    }
    return nodes;
}

unsigned long long engine_perft(ChessBoard *board, int depth) {
    return perft_inner(board, depth);
}

int engine_search_best_move(ChessBoard *board, ChessMove *out_move, int depth) {
    // Use search module to find best move
    return search_best_move(board, out_move, depth);
}

// Iterative deepening: call search_best_move for increasing depths and invoke callback
int engine_search_iterative(ChessBoard *board, ChessMove *out_move, int max_depth, engine_progress_cb cb, void *cb_ctx, int time_limit_ms) {
    if (max_depth <= 0) return 0;

    struct timespec start_ts;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);

    int last_score = 0;
    ChessMove last_move = { .start_tile = -1, .end_tile = -1, .move_type = 0 };

    for (int d = 1; d <= max_depth; ++d) {
        int score = search_best_move(board, &last_move, d);
        last_score = score;

        // invoke progress callback for this completed depth
        if (cb) cb(&last_move, d, score, cb_ctx);

        // check time limit
        if (time_limit_ms > 0) {
            struct timespec now_ts;
            clock_gettime(CLOCK_MONOTONIC, &now_ts);
            unsigned int elapsed = (unsigned int)((now_ts.tv_sec - start_ts.tv_sec) * 1000 + (now_ts.tv_nsec - start_ts.tv_nsec) / 1000000);
            if (elapsed >= (unsigned int)time_limit_ms) break;
        }
    }

    if (out_move && last_move.start_tile >= 0) *out_move = last_move;
    return last_score;
}
