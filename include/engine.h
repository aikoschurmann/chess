// Simple engine framework - high level API
#ifndef ENGINE_H
#define ENGINE_H

#include "chess_board.h"

// Initialize engine subsystem (call once at startup)
void engine_init(void);

// Run a perft test from the given board to the given depth. Returns node count.
unsigned long long engine_perft(ChessBoard *board, int depth);

// Run a best-move search (very small stub). Returns best move in `out_move` and
// a score. This is intentionally minimal — replace with a real search later.
int engine_search_best_move(ChessBoard *board, ChessMove *out_move, int depth);

// Iterative search with progress callback. The callback will be called after
// each completed depth with the current best move and score.
// If time_limit_ms is >0, the search will stop early when the time limit is hit.
// Returns the final score.
typedef void (*engine_progress_cb)(const ChessMove *best_move, int depth, int score, void *ctx);
int engine_search_iterative(ChessBoard *board, ChessMove *out_move, int max_depth, engine_progress_cb cb, void *cb_ctx, int time_limit_ms);

#endif
