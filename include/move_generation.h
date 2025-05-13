#ifndef __MOVE_GENERATION_H__
#define __MOVE_GENERATION_H__
#include "chess_board.h"
#include "chess_bitboard.h"
#include <stdint.h>

ChessMove generate_move(int start_tile, int end_tile, Piece piece_type, Piece promotion, MoveType move_type);

void generate_pawn_moves(ChessBoard *board, ChessMove *moves, int *num_moves, Piece piece_type);
void generate_bishop_moves(ChessBoard *board, ChessMove *moves, int *num_moves, Piece piece_type);
void generate_rook_moves(ChessBoard *board, ChessMove *moves, int *num_moves, Piece piece_type);
void generate_queen_moves(ChessBoard *board, ChessMove *moves, int *num_moves, Piece piece_type);   
void generate_knight_moves(ChessBoard *board, ChessMove *moves, int *num_moves, Piece piece_type);
void generate_king_moves(ChessBoard *board, ChessMove *moves, int *num_moves, Piece piece_type);
void generate_moves(ChessBoard *board, ChessMove *moves, int *num_moves);

#endif // __MOVE_GENERATION_H__