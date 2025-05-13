#include "chess_bitboard.h"

void initialize_bitboards() {
    // Initialize white pieces
    white_pawns = 0x000000000000FF00; // White pawns are on the second rank (8th to 15th bits)
    white_knights = 0x0000000000000042; // White knights are on the b1 and g1 squares (1st and 7th bits)
    white_bishops = 0x0000000000000024; // White bishops are on the c1 and f1 squares (3rd and 6th bits)
    white_rooks = 0x0000000000000081; // White rooks are on a1 and h1 squares (0th and 7th bits)
    white_queens = 0x0000000000000010; // White queen is on d1 (4th bit)
    white_king = 0x0000000000000008; // White king is on e1 (3rd bit)

    // Initialize black pieces (same logic for black pieces)
    black_pawns = 0x00FF000000000000; // Black pawns are on the seventh rank (48th to 55th bits)
    black_knights = 0x4200000000000000; // Black knights are on b8 and g8 (1st and 7th bits)
    black_bishops = 0x2400000000000000; // Black bishops are on c8 and f8 (3rd and 6th bits)
    black_rooks = 0x8100000000000000; // Black rooks are on a8 and h8 (0th and 7th bits)
    black_queens = 0x1000000000000000; // Black queen is on d8 (4th bit)
    black_king = 0x0800000000000000; // Black king is on e8 (3rd bit)

    white_bitboards[PAWN] = white_pawns;
    white_bitboards[KNIGHT] = white_knights;
    white_bitboards[BISHOP] = white_bishops;
    white_bitboards[ROOK] = white_rooks;
    white_bitboards[QUEEN] = white_queens;
    white_bitboards[KING] = white_king;

    black_bitboards[PAWN] = black_pawns;
    black_bitboards[KNIGHT] = black_knights;
    black_bitboards[BISHOP] = black_bishops;
    black_bitboards[ROOK] = black_rooks;
    black_bitboards[QUEEN] = black_queens;
    black_bitboards[KING] = black_king;
}

Piece cycle_bitboard(Piece current_bitboard, int direction) {
    return (current_bitboard + direction + 6) % 6; // Ensures it stays within 0 to 5
}


Bitboard generate_combined(Bitboard *pieces) {
    Bitboard combined = 0;
    for (int i = 0; i < 6; i++) {
        combined |= pieces[i];
    }
    return combined;
}