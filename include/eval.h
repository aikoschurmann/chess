// Simple evaluation functions
#ifndef EVAL_H
#define EVAL_H

#include "chess_board.h"

// Evaluate material balance only (positive = advantage for side to move)
int evaluate_material(ChessBoard *board);

#endif
