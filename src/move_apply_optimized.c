#include "move_apply_optimized.h"
#include "chess_bitboard.h"

// Fast inline piece manipulation
static inline void update_piece_position(ChessBoard *board, ChessColor color, Piece piece, int from, int to) {
    Bitboard *piece_bb = &board->pieces[color][piece];
    *piece_bb &= ~(1ULL << from);  // Remove from old position
    *piece_bb |= (1ULL << to);     // Add to new position
}

static inline void remove_piece(ChessBoard *board, ChessColor color, Piece piece, int square) {
    board->pieces[color][piece] &= ~(1ULL << square);
}

static inline void add_piece(ChessBoard *board, ChessColor color, Piece piece, int square) {
    board->pieces[color][piece] |= (1ULL << square);
}

static inline void update_combined_bitboards(ChessBoard *board) {
    board->combined[WHITE] = generate_combined(board->pieces[WHITE]);
    board->combined[BLACK] = generate_combined(board->pieces[BLACK]);
}

// Fast normal move application
static inline void apply_normal_move_fast(ChessBoard *board, const ChessMove *move) {
    ChessColor color = board->current_turn;
    update_piece_position(board, color, move->piece_type, move->start_tile, move->end_tile);
}

// Fast capture application
static inline void apply_capture_fast(ChessBoard *board, const ChessMove *move, Piece captured) {
    ChessColor us = board->current_turn;
    ChessColor them = us == WHITE ? BLACK : WHITE;
    
    // Move our piece
    update_piece_position(board, us, move->piece_type, move->start_tile, move->end_tile);
    
    // Remove captured piece
    remove_piece(board, them, captured, move->end_tile);
}

// Fast promotion application
static inline void apply_promotion_fast(ChessBoard *board, const ChessMove *move) {
    ChessColor color = board->current_turn;
    
    // Remove pawn
    remove_piece(board, color, PAWN, move->start_tile);
    
    // Add promoted piece
    add_piece(board, color, move->promotion, move->end_tile);
}

// Determine what piece is captured at a square
static inline Piece get_captured_piece(ChessBoard *board, int square) {
    ChessColor enemy = board->current_turn == WHITE ? BLACK : WHITE;
    Bitboard square_mask = 1ULL << square;
    
    for (Piece piece = PAWN; piece <= KING; piece++) {
        if (board->pieces[enemy][piece] & square_mask) {
            return piece;
        }
    }
    return EMPTY;  // Use EMPTY instead of NO_PIECE
}

// Main optimized move application
void apply_move_fast(ChessBoard *board, const ChessMove *move, UndoInfo *undo) {
    // Store undo information
    undo->move = *move;
    undo->prev_en_passant = board->en_passant_tile;
    undo->prev_castling_rights = board->castling_rights.rights;
    undo->captured_square = -1;
    undo->captured_piece = EMPTY;
    
    ChessColor us = board->current_turn;
    ChessColor them = (us == WHITE) ? BLACK : WHITE;
    
    // Handle captures based on move type
    if (move->move_type == MOVE_EN_PASSANT) {
        // En passant: captured pawn is not on end_tile but behind it
        int cap_sq = (us == WHITE) ? (move->end_tile - 8) : (move->end_tile + 8);
        undo->captured_square = cap_sq;
        undo->captured_piece = PAWN;
        // Remove captured pawn
        remove_piece(board, them, PAWN, cap_sq);
    } else {
        // Check for normal capture
        Piece captured = get_captured_piece(board, move->end_tile);
        if (captured != EMPTY) {
            undo->captured_square = move->end_tile;
            undo->captured_piece = captured;
            // Remove captured piece
            remove_piece(board, them, captured, move->end_tile);
        }
    }
    
    // Apply the move based on type
    switch (move->move_type) {
        case MOVE_NORMAL:
            apply_normal_move_fast(board, move);
            break;
            
        case MOVE_PROMOTION:
            apply_promotion_fast(board, move);
            break;
            
        case MOVE_DOUBLE_PUSH:
            apply_normal_move_fast(board, move);
            // Set en passant square to the passed-over square
            board->en_passant_tile = (move->start_tile + move->end_tile) / 2;
            break;
            
        case MOVE_EN_PASSANT:
            // Move pawn (captured pawn already removed above)
            apply_normal_move_fast(board, move);
            break;
            
        case MOVE_CASTLING:
            // Move king
            apply_normal_move_fast(board, move);
            // Move rook
            if (move->rook_location != -1 && move->rook_end_location != -1) {
                update_piece_position(board, board->current_turn, ROOK, 
                                    move->rook_location, move->rook_end_location);
            }
            break;
            
        case MOVE_TYPE_COUNT:
            // This case should never occur - added to silence warnings
            break;
    }
    
    // Clear en passant if not a double push
    if (move->move_type != MOVE_DOUBLE_PUSH) {
        board->en_passant_tile = -1;
    }
    
    // Update castling rights based on moves
    if (move->piece_type == KING) {
        // King moved - lose all castling rights for this color
        if (us == WHITE) {
            board->castling_rights.rights &= ~(WHITE_KINGSIDE | WHITE_QUEENSIDE);
        } else {
            board->castling_rights.rights &= ~(BLACK_KINGSIDE | BLACK_QUEENSIDE);
        }
    } else if (move->piece_type == ROOK) {
        // Rook moved - check which corner
        if (us == WHITE) {
            if (move->start_tile == 0) {  // a1 rook
                board->castling_rights.rights &= ~WHITE_QUEENSIDE;
            } else if (move->start_tile == 7) {  // h1 rook
                board->castling_rights.rights &= ~WHITE_KINGSIDE;
            }
        } else {
            if (move->start_tile == 56) {  // a8 rook
                board->castling_rights.rights &= ~BLACK_QUEENSIDE;
            } else if (move->start_tile == 63) {  // h8 rook
                board->castling_rights.rights &= ~BLACK_KINGSIDE;
            }
        }
    }
    
    // Check if any rooks were captured (affects opponent's castling)
    if (undo->captured_piece == ROOK && undo->captured_square != -1) {
        if (them == WHITE) {
            if (undo->captured_square == 0) {  // a1 rook captured
                board->castling_rights.rights &= ~WHITE_QUEENSIDE;
            } else if (undo->captured_square == 7) {  // h1 rook captured
                board->castling_rights.rights &= ~WHITE_KINGSIDE;
            }
        } else {
            if (undo->captured_square == 56) {  // a8 rook captured
                board->castling_rights.rights &= ~BLACK_QUEENSIDE;
            } else if (undo->captured_square == 63) {  // h8 rook captured
                board->castling_rights.rights &= ~BLACK_KINGSIDE;
            }
        }
    }
    
    // Update combined bitboards
    update_combined_bitboards(board);
    
    // Switch turns
    board->current_turn = them;
    
    validate_board_state(board);
}

// Optimized move undo
void undo_move_fast(ChessBoard *board, const UndoInfo *undo) {
    const ChessMove *move = &undo->move;
    
    // Switch turns back to the side that made the move
    ChessColor them = board->current_turn;  // currently side to move after make
    ChessColor us = (them == WHITE) ? BLACK : WHITE;  // side that made the move
    board->current_turn = us;
    
    // Undo the move based on type - move piece back first
    switch (move->move_type) {
        case MOVE_NORMAL:
        case MOVE_DOUBLE_PUSH:
        case MOVE_EN_PASSANT:
            // Move piece back from end_tile to start_tile
            update_piece_position(board, us, move->piece_type, move->end_tile, move->start_tile);
            break;
            
        case MOVE_PROMOTION:
            // Remove promoted piece
            remove_piece(board, us, move->promotion, move->end_tile);
            // Restore pawn on start square
            add_piece(board, us, PAWN, move->start_tile);
            break;
            
        case MOVE_CASTLING:
            // Move king back
            update_piece_position(board, us, move->piece_type, move->end_tile, move->start_tile);
            // Move rook back
            if (move->rook_location != -1 && move->rook_end_location != -1) {
                update_piece_position(board, us, ROOK, 
                                    move->rook_end_location, move->rook_location);
            }
            break;
            
        case MOVE_TYPE_COUNT:
            // This case should never occur - added to silence warnings
            break;
    }
    
    // Restore captured piece if any
    if (undo->captured_square != -1) {
        int cap_sq = undo->captured_square;
        Piece cap_piece = undo->captured_piece;
        ChessColor cap_color = them;  // the side that was captured
        
        add_piece(board, cap_color, cap_piece, cap_sq);
    }
    
    // Restore previous en passant square
    board->en_passant_tile = undo->prev_en_passant;
    
    // Restore castling rights
    board->castling_rights.rights = undo->prev_castling_rights;
    
    // Update combined bitboards
    update_combined_bitboards(board);
    
    validate_board_state(board);
}


