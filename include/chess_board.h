#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include "chess_bitboard.h"

#define WHITE_KINGSIDE          (1 << 0)
#define WHITE_QUEENSIDE         (1 << 1)
#define BLACK_KINGSIDE          (1 << 2)
#define BLACK_QUEENSIDE         (1 << 3)


typedef struct {
    uint8_t rights;
} CastlingRights;

typedef struct {
    Bitboard pieces[2][6];
    Bitboard combined[2];
    ChessColor current_turn;
    ChessMove last_move;
    int en_passant_tile;
    CastlingRights *castling_rights;
} ChessBoard;

void init_castling_rights(CastlingRights *crights);
void disable_castling_rights(CastlingRights *crights, uint8_t flag);
int can_castle(CastlingRights *crights, uint8_t flag);
void mark_king_and_rook(CastlingRights *crights, ChessColor color);
void mark_rook_moved(CastlingRights *crights, ChessColor color, int kingside);

void initialize_board(ChessBoard *board);

void generate_bitboard_from_moves(ChessMove *moves, int num_moves, unsigned long long *bitboard);


void apply_normal_move(ChessBoard *board, ChessMove *move);
void apply_castling(ChessBoard *board, ChessMove *move);
void apply_en_passant(ChessBoard *board, ChessMove *move);
void apply_double_push(ChessBoard *board, ChessMove *move);
void apply_promotion(ChessBoard *board, ChessMove *move);

void apply_move(ChessBoard *board, const ChessMove *move);

#endif // CHESS_BOARD_H