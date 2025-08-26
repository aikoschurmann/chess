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
    // Set the en-passant target to the square “between” start and end
    b->en_passant_tile = (m->start_tile + m->end_tile) >> 1;
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

void apply_move(ChessBoard *board, const ChessMove *move) {
    // 0) pre-move 
    check_if_piece_captured(board, move);
    board->en_passant_tile = -1; // Reset en passant tile

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
    board->current_turn = !board->current_turn; // Switch turn
    board->last_move = *move; // Save the last move
}

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/*
 * Parse a FEN string into board.
 * Returns 0 on success, -1 on failure (malformed FEN).
 *
 * Expected FEN fields used:
 * 1) piece placement
 * 2) active color (w/b)
 * 3) castling availability (KQkq or -)
 * 4) en-passant target square (e.g. e3 or -)
 *
 * Fields 5 and 6 (halfmove/fullmove) are ignored.
 */
int parse_fen(const char *fen, ChessBoard *board) {
    if (!fen || !board) return -1;

    // Clear piece bitboards
    for (int c = 0; c < 2; ++c)
        for (int p = 0; p < 6; ++p)
            board->pieces[c][p] = 0ULL;

    // piece placement
    const char *p = fen;
    int rank = 7; // start at rank 8 (index 7)
    int file = 0; // file 0 = a

    // parse placement until space
    while (*p && *p != ' ') {
        char ch = *p;
        if (ch == '/') {
            rank--;
            file = 0;
            if (rank < 0) return -1; // too many ranks
        } else if (isdigit((unsigned char)ch)) {
            int skip = ch - '0';
            file += skip;
            if (file > 8) return -1;
        } else {
            if (file >= 8 || rank < 0) return -1;
            int color = (isupper((unsigned char)ch)) ? WHITE : BLACK;
            char lower = (char)tolower((unsigned char)ch);
            int piece;
            switch (lower) {
                case 'p': piece = PAWN;   break;
                case 'n': piece = KNIGHT; break;
                case 'b': piece = BISHOP; break;
                case 'r': piece = ROOK;   break;
                case 'q': piece = QUEEN;  break;
                case 'k': piece = KING;   break;
                default: return -1; // unknown piece letter
            }
            int sq = rank * 8 + file; // bit index: a1=0 .. h8=63
            board->pieces[color][piece] |= (1ULL << sq);
            file++;
        }
        p++;
    }

    if (rank != 0 || file > 8) {
        // Not strictly required to be exactly rank==0 at end (some leniency),
        // but this indicates a possibly malformed placement.
        // We'll accept rank==0 or rank<0? If rank > 0 it's incomplete.
        if (rank > 0) return -1;
    }

    // advance past space
    if (!*p) return -1;
    while (*p == ' ') p++;
    if (!*p) return -1;

    // active color
    char active = *p;
    if (active == 'w' || active == 'W') board->current_turn = WHITE;
    else if (active == 'b' || active == 'B') board->current_turn = BLACK;
    else return -1;
    // move p to after color token
    while (*p && *p != ' ') p++;
    if (!*p) {
        // It's allowed for FEN to end here but we expect further tokens (castling, en-passant)
        // We'll set defaults and return success.
        board->combined[WHITE] = generate_combined(board->pieces[WHITE]);
        board->combined[BLACK] = generate_combined(board->pieces[BLACK]);
        board->en_passant_tile = -1;
        board->last_move.start_tile = -1;
        board->last_move.end_tile = -1;
        board->last_move.piece_type = PAWN;
        board->last_move.promotion = PAWN;
        init_castling_rights(&board->castling_rights); // default all allowed
        return 0;
    }

    // castling availability
    while (*p == ' ') p++;
    if (!*p) return -1;
    // read token until space
    const char *start = p;
    while (*p && *p != ' ') p++;
    size_t len = p - start;
    // reset rights to none and set present ones
    board->castling_rights.rights = 0;
    if (len == 1 && start[0] == '-') {
        // no castling rights
    } else {
        for (size_t i = 0; i < len; ++i) {
            char ccast = start[i];
            if (ccast == 'K') board->castling_rights.rights |= WHITE_KINGSIDE;
            else if (ccast == 'Q') board->castling_rights.rights |= WHITE_QUEENSIDE;
            else if (ccast == 'k') board->castling_rights.rights |= BLACK_KINGSIDE;
            else if (ccast == 'q') board->castling_rights.rights |= BLACK_QUEENSIDE;
            else {
                // ignore unexpected char (could return error but be lenient)
            }
        }
    }

    // en-passant target
    while (*p == ' ') p++;
    if (!*p) {
        board->en_passant_tile = -1;
    } else {
        if (*p == '-') {
            board->en_passant_tile = -1;
        } else {
            // expect file letter then rank digit, e.g. "e3"
            if (!isalpha((unsigned char)p[0]) || !isdigit((unsigned char)p[1])) {
                board->en_passant_tile = -1; // malformed; be permissive
            } else {
                char filec = tolower((unsigned char)p[0]);
                char rankc = p[1];
                if (filec < 'a' || filec > 'h' || rankc < '1' || rankc > '8') {
                    board->en_passant_tile = -1;
                } else {
                    int f = filec - 'a';
                    int r = rankc - '1'; // 0..7
                    board->en_passant_tile = r * 8 + f;
                }
            }
        }
    }

    // house-keeping: combined bitboards & last_move defaults
    board->combined[WHITE] = generate_combined(board->pieces[WHITE]);
    board->combined[BLACK] = generate_combined(board->pieces[BLACK]);

    board->last_move.start_tile = -1;
    board->last_move.end_tile = -1;
    board->last_move.piece_type = PAWN;
    board->last_move.promotion = PAWN;

    return 0;
}
