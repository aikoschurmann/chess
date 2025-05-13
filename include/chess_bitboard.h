#ifndef CHESS_BITBOARD_H
#define CHESS_BITBOARD_H

#include "bitboard.h"


typedef enum Piece {
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING, 
    EMPTY,
    NO_PROMOTION
} Piece;

typedef enum {
    MOVE_NORMAL = 0,
    MOVE_PROMOTION,
    MOVE_DOUBLE_PUSH,
    MOVE_EN_PASSANT,
    MOVE_CASTLING,
    MOVE_TYPE_COUNT
} MoveType;


typedef struct {
    int start_tile;
    int end_tile;
    Piece piece_type;
    Piece promotion;
    int is_castling;
    int rook_location;
    int rook_end_location;
    MoveType move_type;
} ChessMove;


typedef enum ChessColor {
    WHITE,
    BLACK
} ChessColor;

Bitboard white_pawns;
Bitboard white_knights;
Bitboard white_bishops;
Bitboard white_rooks;
Bitboard white_queens;
Bitboard white_king;

Bitboard black_pawns;
Bitboard black_knights;
Bitboard black_bishops;
Bitboard black_rooks;
Bitboard black_queens;
Bitboard black_king;

Bitboard white_pieces;
Bitboard black_pieces;

Bitboard all_pieces;

Bitboard white_bitboards[6];
Bitboard black_bitboards[6];

void initialize_bitboards();
Piece cycle_bitboard(Piece current_bitboard, int direction);
Bitboard generate_combined(Bitboard *pieces);
#endif // CHESS_BITBOARD_H