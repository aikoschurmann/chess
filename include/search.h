// Search API (stub)
#ifndef SEARCH_H
#define SEARCH_H

#include "chess_board.h"

// Search API
// Run an alpha-beta negamax search and return the score.
int search_alpha_beta(ChessBoard *board, int depth, int alpha, int beta);

// Find best move to the given search depth; returns score and fills out_move
int search_best_move(ChessBoard *board, ChessMove *out_move, int depth);

#endif
