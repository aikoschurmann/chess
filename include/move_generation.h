#ifndef __MOVE_GENERATION_H__
#define __MOVE_GENERATION_H__
#include "chess_board.h"
#include "chess_bitboard.h"
#include <stdint.h>

// Helper function to create moves
ChessMove generate_move(int start_tile, int end_tile, Piece piece_type, Piece promotion, MoveType move_type);

// Main move generation functions (all now use optimized system)
void generate_moves(ChessBoard *board, ChessMove *moves, int *num_moves);
void generate_moves_legacy(ChessBoard *board, ChessMove *moves, int *num_moves);
void generate_moves_fast(ChessBoard *board, ChessMove *moves, int *num_moves);
void generate_captures_only(ChessBoard *board, ChessMove *moves, int *num_moves);

// Move application wrapper for GUI
void apply_move_simple(ChessBoard *board, const ChessMove *move);

// Utility functions
void verify_king_safety(ChessBoard *board, ChessMove *moves, int *num_moves);
int is_square_attacked_by(ChessBoard *board, int square, ChessColor by);
void print_board(ChessBoard *board);

#endif // __MOVE_GENERATION_H__