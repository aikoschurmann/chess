#ifndef MOVE_APPLY_OPTIMIZED_H
#define MOVE_APPLY_OPTIMIZED_H

#include "chess_board.h"

// Fast move application and undo
typedef struct {
    ChessMove move;
    int captured_square;    // -1 if no captured piece (for EP we save the pawn square)
    Piece captured_piece;   // piece type that was on captured_square
    int prev_en_passant;    // previous board->en_passant_tile
    uint8_t prev_castling_rights;  // previous castling rights
    uint32_t hash_key;      // For position hashing
} UndoInfo;

// Move application functions
void apply_move_fast(ChessBoard *board, const ChessMove *move, UndoInfo *undo);
void undo_move_fast(ChessBoard *board, const UndoInfo *undo);

// Specialized fast applications for common move types
static inline void apply_normal_move_fast(ChessBoard *board, const ChessMove *move);
static inline void apply_capture_fast(ChessBoard *board, const ChessMove *move, Piece captured);
static inline void apply_promotion_fast(ChessBoard *board, const ChessMove *move);

// Incremental board update functions
static inline void update_piece_position(ChessBoard *board, ChessColor color, Piece piece, int from, int to);
static inline void remove_piece(ChessBoard *board, ChessColor color, Piece piece, int square);
static inline void add_piece(ChessBoard *board, ChessColor color, Piece piece, int square);

// Board state validation (debug builds only)
#ifdef DEBUG
void validate_board_state(ChessBoard *board);
#else
#define validate_board_state(board) ((void)0)
#endif

#endif // MOVE_APPLY_OPTIMIZED_H
