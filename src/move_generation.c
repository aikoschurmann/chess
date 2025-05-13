#include "move_generation.h"

int is_attacked(ChessBoard *board, ChessColor color, int tile) {
    return 0;
}

ChessMove generate_move(int start_tile, int end_tile, Piece piece_type, Piece promotion, MoveType move_type) {
    ChessMove move;
    move.start_tile = start_tile;
    move.end_tile = end_tile;
    move.piece_type = piece_type;
    move.promotion = promotion;
    move.is_castling = 0;
    move.rook_location = -1;
    move.move_type = move_type;
    return move;
}

void generate_pawn_moves(ChessBoard *board, ChessMove *moves, int *num_moves, Piece piece_type) {
    Bitboard pieces = board->pieces[board->current_turn][piece_type];
    ChessColor opposing_color = board->current_turn == WHITE ? BLACK : WHITE;
    Bitboard opposing_pieces = board->combined[opposing_color];
    Bitboard occupied = board->combined[WHITE] | board->combined[BLACK];

    int direction = board->current_turn == WHITE ? 8 : -8;
    Bitboard starting_rank = board->current_turn == WHITE ? 0x000000000000FF00ULL : 0x00FF000000000000ULL;
    Bitboard promotion_rank = board->current_turn == WHITE ? 0xFF00000000000000ULL : 0x00000000000000FFULL;

    while (pieces) {
        int index = __builtin_ctzll(pieces);
        pieces &= pieces - 1;  // Remove lowest set bit

        int forward_tile = index + direction;
        Bitboard forward_mask = 1ULL << forward_tile;

        // Single push and double push
        if (!(occupied & forward_mask)) {
            // Promotion
            if (forward_mask & promotion_rank) {
                moves[(*num_moves)++] = generate_move(index, forward_tile, PAWN, QUEEN, MOVE_PROMOTION);
                moves[(*num_moves)++] = generate_move(index, forward_tile, PAWN, ROOK, MOVE_PROMOTION);
                moves[(*num_moves)++] = generate_move(index, forward_tile, PAWN, BISHOP, MOVE_PROMOTION);
                moves[(*num_moves)++] = generate_move(index, forward_tile, PAWN, KNIGHT, MOVE_PROMOTION);
            } else {
                // Add single push (no promotion)
                moves[(*num_moves)++] = generate_move(index, forward_tile, PAWN, PAWN, MOVE_NORMAL);
                if ((starting_rank & (1ULL << index)) && !(occupied & (1ULL << (forward_tile + direction)))) {

                    moves[(*num_moves)++] = generate_move(index, forward_tile + direction, PAWN, NO_PROMOTION, MOVE_DOUBLE_PUSH);
                }
            }
        }

        // Diagonal capture and En passant
        for (int offset = -1; offset <= 1; offset += 2) {
            int diagonal_tile = forward_tile + offset;
            if ((index & 7) != (offset == -1 ? 0 : 7)) {  // Check for A-file or H-file
                if (opposing_pieces & (1ULL << diagonal_tile)) {
                    moves[(*num_moves)++] = generate_move(index, diagonal_tile, PAWN, NO_PROMOTION, MOVE_NORMAL);
                } else if (board->en_passant_tile == diagonal_tile) {
                    moves[(*num_moves)++] = generate_move(index, diagonal_tile, PAWN, NO_PROMOTION, MOVE_EN_PASSANT);
                }
            }
        }
    }
}

void generate_sliding_moves(ChessBoard *board, ChessMove *moves, int *num_moves, Piece piece_type, int directions[4], unsigned long long masks[4][2]) {
    Bitboard pieces = board->pieces[board->current_turn][piece_type];
    ChessColor opposing_color = board->current_turn == WHITE ? BLACK : WHITE;
    Bitboard opposing_pieces = board->combined[opposing_color];
    Bitboard friendly_pieces = board->combined[board->current_turn];

    while (pieces) {
        int index = __builtin_ctzll(pieces);
        pieces &= pieces - 1;  // Remove the lowest set bit

        for (int i = 0; i < 4; i++) {
            int direction = directions[i];
            int current_tile = index;
            unsigned long long current_mask = 1ULL << current_tile;
            unsigned long long mask1 = masks[i][0];
            unsigned long long mask2 = masks[i][1];

            while (current_tile >= 0 && current_tile < 64) {
                if (current_mask & mask1 || current_mask & mask2) {
                    break;
                }

                current_mask = direction < 0 ? current_mask >> -direction : current_mask << direction;
                current_tile += direction;

                if (friendly_pieces & current_mask) {
                    break;
                }

                if (opposing_pieces & current_mask) {
                    // Capture and break
                    moves[(*num_moves)++] = generate_move(index, current_tile, piece_type, NO_PROMOTION, MOVE_NORMAL);
                    break;
                }
                // Otherwise, add continue sliding
                moves[(*num_moves)++] = generate_move(index, current_tile, piece_type, NO_PROMOTION, MOVE_NORMAL);
            }
        }   
    }
}

void generate_bishop_moves(ChessBoard *board, ChessMove *moves, int *num_moves, Piece piece_type) {
    int directions[4] = {7, 9, -7, -9};
    unsigned long long masks[4][2] = {
        {0x0101010101010101, 0xFF00000000000000}, // left and top
        {0x8080808080808080, 0xFF00000000000000}, // right and top
        {0x8080808080808080, 0x00000000000000FF}, // right and bottom
        {0x0101010101010101, 0x00000000000000FF}  // left and bottom
    };
    generate_sliding_moves(board, moves, num_moves, piece_type, directions, masks);
}

void generate_rook_moves(ChessBoard *board, ChessMove *moves, int *num_moves, Piece piece_type) {
    int directions[4] = {8, -8, 1, -1};
    unsigned long long masks[4][2] = {
        {0xFF00000000000000, 0xFF00000000000000},
        {0x00000000000000FF, 0x00000000000000FF},
        {0x8080808080808080, 0x8080808080808080},
        {0x0101010101010101, 0x0101010101010101}
    };
    generate_sliding_moves(board, moves, num_moves, piece_type, directions, masks);
}

void generate_queen_moves(ChessBoard *board, ChessMove *moves, int *num_moves, Piece piece_type) {
    generate_bishop_moves(board, moves, num_moves, piece_type);
    generate_rook_moves(board, moves, num_moves, piece_type);
}

void generate_knight_moves(ChessBoard *board, ChessMove *moves, int *num_moves, Piece piece_type) {
    Bitboard pieces = board->pieces[board->current_turn][piece_type];
    ChessColor opposing_color = board->current_turn == WHITE ? BLACK : WHITE;
    Bitboard opposing_pieces = board->combined[opposing_color];
    Bitboard friendly_pieces = board->combined[board->current_turn];

    int offsets[8] = {6, 10, 15, 17, -6, -10, -15, -17};
    unsigned long long masks[8][2] = {
        {0x0303030303030303, 0xFF00000000000000},
        {0xC0C0C0C0C0C0C0C0, 0xFF00000000000000},
        {0x0101010101010101, 0xFFFF000000000000},
        {0x8080808080808080, 0xFFFF000000000000},
        {0xC0C0C0C0C0C0C0C0, 0x00000000000000FF},
        {0x0303030303030303, 0x00000000000000FF},
        {0x8080808080808080, 0x000000000000FFFF},
        {0x0101010101010101, 0x000000000000FFFF}
    };

    while (pieces) {
        int index = __builtin_ctzll(pieces);
        pieces &= pieces - 1;  // Remove the lowest set bit

        for (int i = 0; i < 8; i++) {
            int offset = offsets[i];
            int current_tile = index;
            unsigned long long current_mask = 1ULL << current_tile;
            unsigned long long mask1 = masks[i][0];
            unsigned long long mask2 = masks[i][1];

            if (current_mask & mask1 || current_mask & mask2) {
                continue;
            }

            current_mask = offset < 0 ? current_mask >> -offset : current_mask << offset;
            current_tile += offset;

            if (friendly_pieces & current_mask) {
                continue;
            }

            moves[(*num_moves)++] = generate_move(index, current_tile, piece_type, NO_PROMOTION, MOVE_NORMAL);
        }
    }
}

void generate_king_moves(ChessBoard *board, ChessMove *moves, int *num_moves, Piece piece_type) {
    Bitboard pieces = board->pieces[board->current_turn][piece_type];
    ChessColor opposing_color = board->current_turn == WHITE ? BLACK : WHITE;
    Bitboard opposing_pieces = board->combined[opposing_color];
    Bitboard friendly_pieces = board->combined[board->current_turn];
    Bitboard occupied = board->combined[WHITE] | board->combined[BLACK];

    int offsets[8] = {8, -8, 1, -1, 7, 9, -7, -9};
    unsigned long long masks[8] = {
        0xFF00000000000000,  // top rank
        0x00000000000000FF,  // bottom rank
        0x8080808080808080,  // right file
        0x0101010101010101,  // left file
        0XFF01010101010101,  // top and left
        0xFF80808080808080,  // top and right
        0x80000000000000FF,  // bottom and right
        0x01010101010101FF   // bottom and left
    };

    unsigned long long kingside_masks[2];
    kingside_masks[WHITE] = 0x0000000000000060;
    kingside_masks[BLACK] = 0x6000000000000000;

    unsigned long long queenside_masks[2];
    queenside_masks[WHITE] = 0x000000000000000E;
    queenside_masks[BLACK] = 0x0E00000000000000;

    while (pieces) {
        int index = __builtin_ctzll(pieces);
        pieces &= pieces - 1;  // Remove the lowest set bit

        for (int i = 0; i < 8; i++) {
            int offset = offsets[i];
            int current_tile = index;
            unsigned long long current_mask = 1ULL << current_tile;
            unsigned long long mask = masks[i];

            if (current_mask & mask) {
                continue;
            }

            current_mask = offset < 0 ? current_mask >> -offset : current_mask << offset;
            current_tile += offset;

            if (friendly_pieces & current_mask) {
                continue;
            }
        
            moves[(*num_moves)++] = generate_move(index, current_tile, piece_type, NO_PROMOTION, MOVE_NORMAL);
        }


        // Kingside castling (O-O)
        if (can_castle(&board->castling_rights, board->current_turn == WHITE ? WHITE_KINGSIDE : BLACK_KINGSIDE)) {
            // Determine starting king position
            int king_start = (board->current_turn == WHITE) ? 4 : 60;  // E1 for white, E8 for black
            int king_dest = king_start + 2;  // Move to G1 or G8
            int squares[] = {king_start + 1};  // F1, G1 (or F8, G8)

            if (!(occupied & kingside_masks[board->current_turn]) &&
                !is_attacked(board, board->current_turn, king_start) &&
                !is_attacked(board, board->current_turn, squares[0]) &&
                !is_attacked(board, board->current_turn, king_dest)) {

                ChessMove move = generate_move(king_start, king_dest, piece_type, NO_PROMOTION, MOVE_CASTLING);
                move.is_castling = 1;
                move.rook_location = king_start + 3;
                move.rook_end_location = king_start + 1;
                moves[(*num_moves)++] = move;
            }
        }

        // Queenside castling (O-O-O)
        if (can_castle(&board->castling_rights, board->current_turn == WHITE ? WHITE_QUEENSIDE : BLACK_QUEENSIDE)) {
            int king_start = (board->current_turn == WHITE) ? 4 : 60; 
            int king_dest = king_start - 3;  // Move to C1 or C8
            int squares[] = {king_start - 1, king_start - 2}; 

            if (!(occupied & queenside_masks[board->current_turn]) &&
                !is_attacked(board, board->current_turn, king_start) &&
                !is_attacked(board, board->current_turn, squares[0]) &&
                !is_attacked(board, board->current_turn, squares[1]) &&
                !is_attacked(board, board->current_turn, king_dest)) {

                ChessMove move = generate_move(king_start, king_dest, piece_type, NO_PROMOTION, MOVE_CASTLING);
                move.is_castling = 1;
                move.rook_location = king_start - 4;
                move.rook_end_location = king_start - 1;
                moves[(*num_moves)++] = move;
            }
        }
    }
}


void generate_moves(ChessBoard *board, ChessMove *moves, int *num_moves) {
    *num_moves = 0;
    generate_pawn_moves(board, moves, num_moves, PAWN);
    generate_bishop_moves(board, moves, num_moves, BISHOP);
    generate_rook_moves(board, moves, num_moves, ROOK);
    generate_queen_moves(board, moves, num_moves, QUEEN);
    generate_knight_moves(board, moves, num_moves, KNIGHT);
    generate_king_moves(board, moves, num_moves, KING);
}