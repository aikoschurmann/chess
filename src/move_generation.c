#include "move_generation.h"
#include "move_gen_optimized.h"
#include "move_apply_optimized.h"
#include "magic_attacks.h"
#include "magic_bitboards.h"
#include "bitboard.h"
#include <stdio.h>
#include <string.h>

// Simple attack detection using magic bitboards
// File-mask constants (place near top of file)
static const Bitboard FILE_A = 0x0101010101010101ULL;
static const Bitboard FILE_H = 0x8080808080808080ULL;

// --- is_square_attacked_by ---
// Returns 1 if the square is attacked by color `by`, 0 otherwise.
int is_square_attacked_by(ChessBoard *board, int square, ChessColor by) {
    if (square < 0 || square > 63) return 0;
    Bitboard occ = board->combined[WHITE] | board->combined[BLACK];
    Bitboard bb = 1ULL << square;

    // Pawns
    if (by == WHITE) {
        // white pawns attack to (p+7) and (p+9), so to see if any white pawn attacks 'square'
        // build white pawn attack bitboard and test membership of 'square'
        Bitboard white_pawns = board->pieces[WHITE][PAWN];
        Bitboard attacks_from_white = 0;
        // pawns << 7 attacks (avoid wrapping from file H)
        attacks_from_white |= (white_pawns << 7) & ~FILE_H;
        // pawns << 9 attacks (avoid wrapping from file A)
        attacks_from_white |= (white_pawns << 9) & ~FILE_A;
        if (attacks_from_white & bb) return 1;
    } else {
        // black pawns attack to (p-7) and (p-9) -> use >> shifts
        Bitboard black_pawns = board->pieces[BLACK][PAWN];
        Bitboard attacks_from_black = 0;
        attacks_from_black |= (black_pawns >> 7) & ~FILE_A;
        attacks_from_black |= (black_pawns >> 9) & ~FILE_H;
        if (attacks_from_black & bb) return 1;
    }

    // Knights
    if (movegen_knight_attacks(square) & board->pieces[by][KNIGHT]) return 1;

    // Bishops & Queens (diagonal sliders)
    Bitboard diag_attackers = get_bishop_attack(square, occ) & (board->pieces[by][BISHOP] | board->pieces[by][QUEEN]);
    if (diag_attackers) return 1;

    // Rooks & Queens (orthogonal sliders)
    Bitboard orth_attackers = get_rook_attack(square, occ) & (board->pieces[by][ROOK] | board->pieces[by][QUEEN]);
    if (orth_attackers) return 1;

    // King (adjacent)
    if (movegen_king_attacks(square) & board->pieces[by][KING]) return 1;

    return 0;
}

// Main move generation function - now uses optimized system
void generate_moves(ChessBoard *board, ChessMove *moves, int *num_moves) {
    *num_moves = generate_moves_optimized(board, moves, MOVEGEN_ALL);
}

// Fast move generation (same as main function now)
void generate_moves_fast(ChessBoard *board, ChessMove *moves, int *num_moves) {
    *num_moves = generate_moves_optimized(board, moves, MOVEGEN_ALL);
}

// Capture-only move generation for quiescence search
void generate_captures_only(ChessBoard *board, ChessMove *moves, int *num_moves) {
    *num_moves = generate_moves_optimized(board, moves, MOVEGEN_CAPTURES);
}

// Simple move application wrapper for GUI (no undo info needed)
void apply_move_simple(ChessBoard *board, const ChessMove *move) {
    UndoInfo undo; // We don't use this in GUI, but function requires it
    apply_move_fast(board, move, &undo);
}

// Helper function to create moves (kept for compatibility)
ChessMove generate_move(int start_tile, int end_tile, Piece piece_type, Piece promotion, MoveType move_type) {
    ChessMove move;
    move.start_tile = start_tile;
    move.end_tile = end_tile;
    move.piece_type = piece_type;
    move.promotion = promotion;
    move.is_castling = 0;
    move.rook_location = -1;
    move.rook_end_location = -1;
    move.move_type = move_type;
    return move;
}

#define NUM_PIECE_TYPES 6

void verify_king_safety(ChessBoard *board, ChessMove *moves, int *num_moves) {
    static ChessMove verified_moves[512];
    int verified_count = 0;

    ChessColor us = board->current_turn;
    ChessColor opponent = (us == WHITE) ? BLACK : WHITE;

    // original king square (fast path for non-king moves)
    Bitboard orig_king_bb = board->pieces[us][KING];
    if (!orig_king_bb) { // defensive: no king (shouldn't happen)
        *num_moves = 0;
        return;
    }
    int orig_king_sq = __builtin_ctzll(orig_king_bb);

    int n = *num_moves;
    for (int i = 0; i < n; ++i) {
        ChessMove *m = &moves[i];

        // Fast determine where the king will be after the move
        int king_sq_after = (m->piece_type == KING) ? m->end_tile : orig_king_sq;
        if (king_sq_after < 0) continue; // defensive

        // Save minimal state that can change.
        // Save both color piece arrays (6 bitboards each) and combined bitboards,
        // plus en_passant and castling_rights bytes.
        #define NUM_PIECE_TYPES 6
        Bitboard saved_us[NUM_PIECE_TYPES];
        Bitboard saved_them[NUM_PIECE_TYPES];
        Bitboard saved_comb_us = board->combined[us];
        Bitboard saved_comb_them = board->combined[opponent];
        int saved_en_passant = board->en_passant_tile;

        unsigned char saved_castling_bytes[32];
        size_t castling_size = sizeof(board->castling_rights);
        if (castling_size > sizeof(saved_castling_bytes)) castling_size = sizeof(saved_castling_bytes);
        memcpy(saved_castling_bytes, &board->castling_rights, castling_size);

        for (int p = 0; p < NUM_PIECE_TYPES; ++p) {
            saved_us[p] = board->pieces[us][p];
            saved_them[p] = board->pieces[opponent][p];
        }

        // Apply move incrementally (piece bitboards + combined + en_passant)
        Bitboard from_bb = 1ULL << m->start_tile;
        Bitboard to_bb   = 1ULL << m->end_tile;

        // Remove moving piece from origin
        board->pieces[us][m->piece_type] &= ~from_bb;
        board->combined[us] &= ~from_bb;

        // Handle captures (including en-passant)
        int captured_square = -1;
        int captured_piece_type = -1;

        if (m->move_type == MOVE_EN_PASSANT) {
            int cap_sq = (us == WHITE) ? (m->end_tile - 8) : (m->end_tile + 8);
            Bitboard cap_bb = 1ULL << cap_sq;
            board->pieces[opponent][PAWN] &= ~cap_bb;
            board->combined[opponent] &= ~cap_bb;
            captured_square = cap_sq;
            captured_piece_type = PAWN;
        } else {
            if (board->combined[opponent] & to_bb) {
                for (int p = 0; p < NUM_PIECE_TYPES; ++p) {
                    if (board->pieces[opponent][p] & to_bb) {
                        board->pieces[opponent][p] &= ~to_bb;
                        board->combined[opponent] &= ~to_bb;
                        captured_square = m->end_tile;
                        captured_piece_type = p;
                        break;
                    }
                }
            }
        }

        // Place moving piece at destination (promotions handled)
        if (m->promotion != NO_PROMOTION) {
            board->pieces[us][m->promotion] |= to_bb;
            board->combined[us] |= to_bb;
        } else {
            board->pieces[us][m->piece_type] |= to_bb;
            board->combined[us] |= to_bb;
        }

        // Handle castling rook move if present
        if (m->is_castling) {
            Bitboard rook_from_bb = 1ULL << m->rook_location;
            Bitboard rook_to_bb   = 1ULL << m->rook_end_location;
            board->pieces[us][ROOK] &= ~rook_from_bb;
            board->pieces[us][ROOK] |= rook_to_bb;
            board->combined[us] &= ~rook_from_bb;
            board->combined[us] |= rook_to_bb;
        }

        // Update en_passant for double pawn pushes, otherwise clear
        board->en_passant_tile = -1;
        if (m->move_type == MOVE_DOUBLE_PUSH) {
            if (us == WHITE) board->en_passant_tile = m->start_tile + 8;
            else              board->en_passant_tile = m->start_tile - 8;
        }

        // Note: we DO NOT mutate castling_rights here. That avoids heuristic ifdefs.
        // The saved_castling_bytes will be restored below (no change during the test).

        // Flip turn so is_square_attacked_by sees correct attacker side
        board->current_turn = opponent;

        // Now test king safety using the canonical attacker test
        int king_safe = !is_square_attacked_by(board, king_sq_after, opponent);

        // Restore everything we changed
        for (int p = 0; p < NUM_PIECE_TYPES; ++p) {
            board->pieces[us][p] = saved_us[p];
            board->pieces[opponent][p] = saved_them[p];
        }
        board->combined[us] = saved_comb_us;
        board->combined[opponent] = saved_comb_them;
        board->en_passant_tile = saved_en_passant;
        memcpy(&board->castling_rights, saved_castling_bytes, castling_size);
        board->current_turn = us;

        if (king_safe) {
            verified_moves[verified_count++] = *m;
        }

        #undef NUM_PIECE_TYPES
    }

    // Copy verified moves back
    for (int i = 0; i < verified_count; ++i) moves[i] = verified_moves[i];
    *num_moves = verified_count;
}