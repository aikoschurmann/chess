#include "chess_board.h"
#include <stdio.h>
#include <math.h>



int abs(int x) {
    return x < 0 ? -x : x;
}

void initialize_board(ChessBoard *board) {
    // Initialize white pieces
    board->pieces[WHITE][PAWN] = 0x000000000000FF00;
    board->pieces[WHITE][KNIGHT] = 0x0000000000000042;
    board->pieces[WHITE][BISHOP] = 0x0000000000000024;
    board->pieces[WHITE][ROOK] = 0x0000000000000081;
    board->pieces[WHITE][QUEEN] = 0x0000000000000008;
    board->pieces[WHITE][KING] = 0x0000000000000010;

    // Initialize black pieces
    board->pieces[BLACK][PAWN] = 0x00FF000000000000;
    board->pieces[BLACK][KNIGHT] = 0x4200000000000000;
    board->pieces[BLACK][BISHOP] = 0x2400000000000000;
    board->pieces[BLACK][ROOK] = 0x8100000000000000;
    board->pieces[BLACK][QUEEN] = 0x0800000000000000;
    board->pieces[BLACK][KING] = 0x1000000000000000;

    board->combined[WHITE] = generate_combined(board->pieces[WHITE]);
    board->combined[BLACK] = generate_combined(board->pieces[BLACK]);

    board->current_turn = WHITE;
    board->last_move.start_tile = -1;
    board->last_move.end_tile = -1;
    board->last_move.piece_type = PAWN;
    board->last_move.promotion = PAWN;
    board->en_passant_tile = -1;

    init_castling_rights(&board->castling_rights);
}

void init_castling_rights(CastlingRights *crights) {
    crights->rights = WHITE_KINGSIDE | WHITE_QUEENSIDE | BLACK_KINGSIDE | BLACK_QUEENSIDE;
}

void disable_castling_rights(CastlingRights *crights, uint8_t flag) {
    crights->rights &= ~flag;
}

// Return non-zero if the specified castling right is still available
int can_castle(CastlingRights *crights, uint8_t flag) {
    return (crights->rights & flag) != 0;
}


void mark_king_moved(CastlingRights *crights, ChessColor color) {
    uint8_t flag = color == WHITE ? (WHITE_KINGSIDE | WHITE_QUEENSIDE) : (BLACK_KINGSIDE | BLACK_QUEENSIDE);
    disable_castling_rights(crights, flag);
}

void mark_rook_moved(CastlingRights *crights, ChessColor color, int kingside) {
    uint8_t flag = color == WHITE
         ? (kingside ? WHITE_KINGSIDE : WHITE_QUEENSIDE)
         : (kingside ? BLACK_KINGSIDE : BLACK_QUEENSIDE);
    disable_castling_rights(crights, flag);
}

void generate_bitboard_from_moves(ChessMove *moves, int num_moves, unsigned long long *bitboard) {
    *bitboard = 0x0000000000000000;
    for (int i = 0; i < num_moves; i++) {
        ChessMove move = moves[i];
        *bitboard |= 1ULL << move.end_tile;
    }
}

void check_if_piece_captured(ChessBoard *board, ChessMove *move) {
    Piece captured_piece = EMPTY;
    for (int i = 0; i < 6; i++) {
        if (board->pieces[!board->current_turn][i] & (1ULL << move->end_tile)) {
            captured_piece = i;
            break;
        }
    }

    if (captured_piece != EMPTY) {
        board->pieces[!board->current_turn][captured_piece] &= ~(1ULL << move->end_tile);
    }
}

static void (* const move_handlers[MOVE_TYPE_COUNT])(ChessBoard*, const ChessMove*) = {
    [MOVE_NORMAL]      = apply_normal_move,
    [MOVE_PROMOTION]   = apply_promotion,
    [MOVE_DOUBLE_PUSH] = apply_double_push,
    [MOVE_EN_PASSANT]  = apply_en_passant,
    [MOVE_CASTLING]    = apply_castling,
};

void apply_normal_move  (ChessBoard *b, ChessMove *m) {
    b->pieces[b->current_turn][m->piece_type] &= ~(1ULL << m->start_tile);
    b->pieces[b->current_turn][m->piece_type] |= 1ULL << m->end_tile;
}

void apply_promotion    (ChessBoard *b, ChessMove *m) {
    b->pieces[b->current_turn][PAWN] &= ~(1ULL << m->start_tile);
    b->pieces[b->current_turn][m->promotion] |= 1ULL << m->end_tile;
}

void apply_double_push  (ChessBoard *b, ChessMove *m) {
    b->pieces[b->current_turn][PAWN] &= ~(1ULL << m->start_tile);
    b->pieces[b->current_turn][PAWN] |= 1ULL << m->end_tile;
    b->en_passant_tile = (m->start_tile + m->end_tile) / 2;
}

void apply_en_passant   (ChessBoard *b, ChessMove *m) {
    b->pieces[b->current_turn][PAWN] &= ~(1ULL << m->start_tile);
    b->pieces[b->current_turn][PAWN] |= 1ULL << m->end_tile;
    int captured_pawn = b->current_turn == WHITE ? m->end_tile - 8 : m->end_tile + 8;
    b->pieces[!b->current_turn][PAWN] &= ~(1ULL << captured_pawn);
}

void apply_castling(ChessBoard *b, ChessMove *m) {
    // Move king
    b->pieces[b->current_turn][KING] &= ~(1ULL << m->start_tile);
    b->pieces[b->current_turn][KING] |= 1ULL << m->end_tile;

    // Move rook
    b->pieces[b->current_turn][ROOK] &= ~(1ULL << m->rook_location);
    b->pieces[b->current_turn][ROOK] |= 1ULL << m->rook_end_location;
}

void clear_castling(CastlingRights *cr, ChessColor color, Piece piece, int file) {
    // If the king moves at all, both rights for that side go away
    if (piece == KING) {
        mark_king_moved(cr, color);
        return;
    }
    // If a rook moves from its home corner (a1/h1 or a8/h8), that specific right goes away
    if (piece == ROOK) {
        if (file == 0) {
            // rook from the a‐file: queenside
            mark_rook_moved(cr, color, 0);
        }
        else if (file == 7) {
            // rook from the h‐file: kingside
            mark_rook_moved(cr, color, 1);
        }
    }
}

void apply_move(ChessBoard *board, ChessMove *move) {
    // 0) Handle capture before moving
    check_if_piece_captured(board, move);

    // 1) clear any castling rights impacted by king/rook movement
    clear_castling(&board->castling_rights,
                   board->current_turn,
                   move->piece_type,
                   move->start_tile % 8);

    // 2) dispatch into the one specialized handler
    move_handlers[move->move_type](board, move);

    // 3) shared housekeeping
    board->combined[WHITE] = generate_combined(board->pieces[WHITE]);
    board->combined[BLACK] = generate_combined(board->pieces[BLACK]);
    board->current_turn = !board->current_turn;
}