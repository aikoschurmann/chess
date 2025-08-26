#ifndef MOVE_GEN_OPTIMIZED_H
#define MOVE_GEN_OPTIMIZED_H

#include "chess_board.h"
#include "magic_bitboards.h"
#include <stdint.h>

// Move generation stages for efficiency
typedef enum {
    MOVEGEN_ALL,        // Generate all moves
    MOVEGEN_CAPTURES,   // Only captures and promotions
    MOVEGEN_QUIET,      // Only quiet moves
    MOVEGEN_CHECKS      // Only checking moves
} MoveGenStage;

// Move list with capacity and count
typedef struct {
    ChessMove *moves;
    int count;
    int capacity;
} MoveList;

// Optimized move generation context
typedef struct {
    ChessBoard *board;
    Bitboard our_pieces;
    Bitboard enemy_pieces;
    Bitboard all_pieces;
    Bitboard enemy_king;
    ChessColor us;
    ChessColor them;
    int king_square;
    Bitboard pinned;            // Pinned pieces mask
    Bitboard checkers;          // Pieces giving check
    int check_count;            // Number of checkers (0, 1, or 2)
} MoveGenContext;

// Function declarations
void movegen_init_context(MoveGenContext *ctx, ChessBoard *board);
int movegen_generate_all(MoveGenContext *ctx, ChessMove *moves);
int movegen_generate_captures(MoveGenContext *ctx, ChessMove *moves);
int movegen_generate_quiet(MoveGenContext *ctx, ChessMove *moves);

// Piece-specific optimized generators
// Main optimized move generation function
int generate_moves_optimized(ChessBoard *board, ChessMove *moves, MoveGenStage stage);

// Individual piece move generators
int movegen_pawn_moves(MoveGenContext *ctx, ChessMove *moves, MoveGenStage stage);
int movegen_knight_moves(MoveGenContext *ctx, ChessMove *moves, MoveGenStage stage);
int movegen_bishop_moves(MoveGenContext *ctx, ChessMove *moves, MoveGenStage stage);
int movegen_rook_moves(MoveGenContext *ctx, ChessMove *moves, MoveGenStage stage);
int movegen_queen_moves(MoveGenContext *ctx, ChessMove *moves, MoveGenStage stage);
int movegen_king_moves(MoveGenContext *ctx, ChessMove *moves, MoveGenStage stage);

// Utility functions
Bitboard movegen_knight_attacks(int square);
Bitboard movegen_king_attacks(int square);
static inline int movegen_add_moves(ChessMove *moves, int from, Bitboard targets, Piece piece, MoveType type);

// Attack/pin detection
Bitboard movegen_find_pinned_pieces(MoveGenContext *ctx);
Bitboard movegen_find_checkers(MoveGenContext *ctx);
int movegen_is_square_attacked(MoveGenContext *ctx, int square, ChessColor by_color);

#endif // MOVE_GEN_OPTIMIZED_H
