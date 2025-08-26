#include "move_gen_optimized.h"
#include "move_generation.h"
#include <string.h>

// Helper function to create moves
static inline ChessMove create_move(int from, int to, Piece piece, Piece promotion, MoveType move_type);

// Forward declarations for castling helpers
static int generate_castling_moves(MoveGenContext *ctx, ChessMove *moves);
static int is_castling_path_clear_and_safe(MoveGenContext *ctx, int king_from, int king_to, 
                                          int rook_to, int rook_from, int check_square);

// Helper function to create moves
static inline ChessMove create_move(int from, int to, Piece piece, Piece promotion, MoveType move_type) {
    ChessMove move;
    move.start_tile = from;
    move.end_tile = to;
    move.piece_type = piece;
    move.promotion = promotion;
    move.is_castling = 0;
    move.rook_location = -1;
    move.rook_end_location = -1;
    move.move_type = move_type;
    return move;
}


static Bitboard KNIGHT_ATTACKS[64];
static Bitboard KING_ATTACKS[64];
static int initialized = 0;

// Initialize lookup tables
static void init_attack_tables() {
    if (initialized) return;
    
    // Knight attacks
    for (int sq = 0; sq < 64; sq++) {
        Bitboard attacks = 0;
        int rank = sq / 8;
        int file = sq % 8;
        
        int knight_moves[8][2] = {{-2,-1}, {-2,1}, {-1,-2}, {-1,2}, {1,-2}, {1,2}, {2,-1}, {2,1}};
        
        for (int i = 0; i < 8; i++) {
            int new_rank = rank + knight_moves[i][0];
            int new_file = file + knight_moves[i][1];
            if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8) {
                attacks |= 1ULL << (new_rank * 8 + new_file);
            }
        }
        KNIGHT_ATTACKS[sq] = attacks;
    }
    
    // King attacks
    for (int sq = 0; sq < 64; sq++) {
        Bitboard attacks = 0;
        int rank = sq / 8;
        int file = sq % 8;
        
        for (int dr = -1; dr <= 1; dr++) {
            for (int df = -1; df <= 1; df++) {
                if (dr == 0 && df == 0) continue;
                int new_rank = rank + dr;
                int new_file = file + df;
                if (new_rank >= 0 && new_rank < 8 && new_file >= 0 && new_file < 8) {
                    attacks |= 1ULL << (new_rank * 8 + new_file);
                }
            }
        }
        KING_ATTACKS[sq] = attacks;
    }
    
    initialized = 1;
}

// Fast inline functions
Bitboard movegen_knight_attacks(int square) {
    return KNIGHT_ATTACKS[square];
}

Bitboard movegen_king_attacks(int square) {
    return KING_ATTACKS[square];
}

// Efficient move adding with bitboard iteration
static inline int movegen_add_moves(ChessMove *moves, int from, Bitboard targets, Piece piece, MoveType type) {
    int count = 0;
    while (targets) {
        int to = __builtin_ctzll(targets);
        targets &= targets - 1;  // Remove lowest bit
        
        ChessMove move = {
            .start_tile = from,
            .end_tile = to,
            .piece_type = piece,
            .promotion = NO_PROMOTION,
            .is_castling = 0,
            .rook_location = -1,
            .rook_end_location = -1,
            .move_type = type
        };
        moves[count++] = move;
    }
    return count;
}

// Initialize move generation context with precomputed values
void movegen_init_context(MoveGenContext *ctx, ChessBoard *board) {
    init_attack_tables();
    
    ctx->board = board;
    ctx->us = board->current_turn;
    ctx->them = ctx->us == WHITE ? BLACK : WHITE;
    
    ctx->our_pieces = board->combined[ctx->us];
    ctx->enemy_pieces = board->combined[ctx->them];
    ctx->all_pieces = ctx->our_pieces | ctx->enemy_pieces;
    
    ctx->enemy_king = board->pieces[ctx->them][KING];
    
    // Find our king
    Bitboard our_king = board->pieces[ctx->us][KING];
    if (our_king == 0) {
        // Defensive: avoid undefined ctzll if king missing; set to -1
        ctx->king_square = -1;
    } else {
        ctx->king_square = __builtin_ctzll(our_king);
    }
    
    // Compute pins and checks
    ctx->pinned = movegen_find_pinned_pieces(ctx);
    ctx->checkers = movegen_find_checkers(ctx);
    ctx->check_count = __builtin_popcountll(ctx->checkers);
}

// Optimized pawn move generation
int movegen_pawn_moves(MoveGenContext *ctx, ChessMove *moves, MoveGenStage stage) {
    int count = 0;
    Bitboard pawns = ctx->board->pieces[ctx->us][PAWN];
    
    int forward = (ctx->us == WHITE) ? 8 : -8;
    int start_rank = (ctx->us == WHITE) ? 1 : 6;
    int promotion_rank = (ctx->us == WHITE) ? 7 : 0;
    
    // Forward moves
    if (stage == MOVEGEN_ALL || stage == MOVEGEN_QUIET) {
        Bitboard single_push = (ctx->us == WHITE) ? 
            (pawns << 8) & ~ctx->all_pieces :
            (pawns >> 8) & ~ctx->all_pieces;
            
        Bitboard double_push = (ctx->us == WHITE) ?
            ((single_push & 0x0000000000FF0000ULL) << 8) & ~ctx->all_pieces :
            ((single_push & 0x0000FF0000000000ULL) >> 8) & ~ctx->all_pieces;
        
        // Single pushes
        while (single_push) {
            int to = __builtin_ctzll(single_push);
            single_push &= single_push - 1;
            int from = to - forward;
            
            if (to / 8 == promotion_rank) {
                // Promotions
                if (stage == MOVEGEN_ALL) {
                    ChessMove m = create_move(from, to, PAWN, QUEEN, MOVE_PROMOTION);
                    moves[count++] = m;
                    m.promotion = ROOK; moves[count++] = m;
                    m.promotion = BISHOP; moves[count++] = m;
                    m.promotion = KNIGHT; moves[count++] = m;
                }
            } else {
                ChessMove move = create_move(from, to, PAWN, NO_PROMOTION, MOVE_NORMAL);
                moves[count++] = move;
            }
        }
        
        // Double pushes
        while (double_push) {
            int to = __builtin_ctzll(double_push);
            double_push &= double_push - 1;
            int from = to - 2 * forward;
            
            ChessMove move = create_move(from, to, PAWN, NO_PROMOTION, MOVE_DOUBLE_PUSH);
            moves[count++] = move;
        }
    }
    
    // Captures
    if (stage == MOVEGEN_ALL || stage == MOVEGEN_CAPTURES) {
        Bitboard left_attacks = (ctx->us == WHITE) ?
            ((pawns & 0xFEFEFEFEFEFEFEFEULL) << 7) & ctx->enemy_pieces :
            ((pawns & 0xFEFEFEFEFEFEFEFEULL) >> 9) & ctx->enemy_pieces;
            
        Bitboard right_attacks = (ctx->us == WHITE) ?
            ((pawns & 0x7F7F7F7F7F7F7F7FULL) << 9) & ctx->enemy_pieces :
            ((pawns & 0x7F7F7F7F7F7F7F7FULL) >> 7) & ctx->enemy_pieces;
        
        // Left captures
        while (left_attacks) {
            int to = __builtin_ctzll(left_attacks);
            left_attacks &= left_attacks - 1;
            int from = to - ((ctx->us == WHITE) ? 7 : -9);
            
            if (to / 8 == promotion_rank) {
                ChessMove m = create_move(from, to, PAWN, QUEEN, MOVE_PROMOTION);
                moves[count++] = m;
                m.promotion = ROOK; moves[count++] = m;
                m.promotion = BISHOP; moves[count++] = m;
                m.promotion = KNIGHT; moves[count++] = m;
            } else {
                ChessMove move = create_move(from, to, PAWN, NO_PROMOTION, MOVE_NORMAL);
                moves[count++] = move;
            }
        }
        
        // Right captures
        while (right_attacks) {
            int to = __builtin_ctzll(right_attacks);
            right_attacks &= right_attacks - 1;
            int from = to - ((ctx->us == WHITE) ? 9 : -7);
            
            if (to / 8 == promotion_rank) {
                ChessMove m = create_move(from, to, PAWN, QUEEN, MOVE_PROMOTION);
                moves[count++] = m;
                m.promotion = ROOK; moves[count++] = m;
                m.promotion = BISHOP; moves[count++] = m;
                m.promotion = KNIGHT; moves[count++] = m;
            } else {
                ChessMove move = create_move(from, to, PAWN, NO_PROMOTION, MOVE_NORMAL);
                moves[count++] = move;
            }
        }
        
        // En passant captures
        if (ctx->board->en_passant_tile != -1) {
            int ep_square = ctx->board->en_passant_tile;
            int ep_rank = ep_square / 8;
            int ep_file = ep_square % 8;
            
            // Check if we have pawns that can capture en passant
            Bitboard ep_attackers = 0;
            if (ctx->us == WHITE) {
                // White pawns that can capture to ep_square are on ep_square -7 (from the right)
                // or ep_square -9 (from the left), but file bounds must be checked.
                if (ep_file < 7) {
                    int from = ep_square - 7;
                    if (from >= 0 && (pawns & (1ULL << from))) ep_attackers |= (1ULL << from);
                }
                if (ep_file > 0) {
                    int from = ep_square - 9;
                    if (from >= 0 && (pawns & (1ULL << from))) ep_attackers |= (1ULL << from);
                }
            } else { // BLACK
                // Black pawns capture downwards: attackers are ep_square +7 / +9
                if (ep_file > 0) {
                    int from = ep_square + 7;
                    if (from < 64 && (pawns & (1ULL << from))) ep_attackers |= (1ULL << from);
                }
                if (ep_file < 7) {
                    int from = ep_square + 9;
                    if (from < 64 && (pawns & (1ULL << from))) ep_attackers |= (1ULL << from);
                }
            }
            
            while (ep_attackers) {
                int from = __builtin_ctzll(ep_attackers);
                ep_attackers &= ep_attackers - 1;
                
                ChessMove move = create_move(from, ep_square, PAWN, NO_PROMOTION, MOVE_EN_PASSANT);
                moves[count++] = move;
            }
        }
    }
    
    return count;
}

// Optimized knight move generation
int movegen_knight_moves(MoveGenContext *ctx, ChessMove *moves, MoveGenStage stage) {
    int count = 0;
    Bitboard knights = ctx->board->pieces[ctx->us][KNIGHT];
    
    while (knights) {
        int from = __builtin_ctzll(knights);
        knights &= knights - 1;
        
        Bitboard attacks = movegen_knight_attacks(from) & ~ctx->our_pieces;
        
        if (stage == MOVEGEN_CAPTURES || stage == MOVEGEN_ALL) {
            Bitboard captures = attacks & ctx->enemy_pieces;
            count += movegen_add_moves(moves + count, from, captures, KNIGHT, MOVE_NORMAL);
        }
        
        if (stage == MOVEGEN_QUIET || stage == MOVEGEN_ALL) {
            Bitboard quiet = attacks & ~ctx->enemy_pieces;
            count += movegen_add_moves(moves + count, from, quiet, KNIGHT, MOVE_NORMAL);
        }
    }
    
    return count;
}

// Optimized sliding piece move generation
int movegen_bishop_moves(MoveGenContext *ctx, ChessMove *moves, MoveGenStage stage) {
    int count = 0;
    Bitboard bishops = ctx->board->pieces[ctx->us][BISHOP];
    
    while (bishops) {
        int from = __builtin_ctzll(bishops);
        bishops &= bishops - 1;
        
        Bitboard attacks = get_bishop_attack(from, ctx->all_pieces) & ~ctx->our_pieces;
        
        if (stage == MOVEGEN_CAPTURES || stage == MOVEGEN_ALL) {
            Bitboard captures = attacks & ctx->enemy_pieces;
            count += movegen_add_moves(moves + count, from, captures, BISHOP, MOVE_NORMAL);
        }
        
        if (stage == MOVEGEN_QUIET || stage == MOVEGEN_ALL) {
            Bitboard quiet = attacks & ~ctx->enemy_pieces;
            count += movegen_add_moves(moves + count, from, quiet, BISHOP, MOVE_NORMAL);
        }
    }
    
    return count;
}

int movegen_rook_moves(MoveGenContext *ctx, ChessMove *moves, MoveGenStage stage) {
    int count = 0;
    Bitboard rooks = ctx->board->pieces[ctx->us][ROOK];
    
    while (rooks) {
        int from = __builtin_ctzll(rooks);
        rooks &= rooks - 1;
        
        Bitboard attacks = get_rook_attack(from, ctx->all_pieces) & ~ctx->our_pieces;
        
        if (stage == MOVEGEN_CAPTURES || stage == MOVEGEN_ALL) {
            Bitboard captures = attacks & ctx->enemy_pieces;
            count += movegen_add_moves(moves + count, from, captures, ROOK, MOVE_NORMAL);
        }
        
        if (stage == MOVEGEN_QUIET || stage == MOVEGEN_ALL) {
            Bitboard quiet = attacks & ~ctx->enemy_pieces;
            count += movegen_add_moves(moves + count, from, quiet, ROOK, MOVE_NORMAL);
        }
    }
    
    return count;
}

int movegen_queen_moves(MoveGenContext *ctx, ChessMove *moves, MoveGenStage stage) {
    int count = 0;
    Bitboard queens = ctx->board->pieces[ctx->us][QUEEN];
    
    while (queens) {
        int from = __builtin_ctzll(queens);
        queens &= queens - 1;
        
        Bitboard attacks = get_queen_attack(from, ctx->all_pieces) & ~ctx->our_pieces;
        
        if (stage == MOVEGEN_CAPTURES || stage == MOVEGEN_ALL) {
            Bitboard captures = attacks & ctx->enemy_pieces;
            count += movegen_add_moves(moves + count, from, captures, QUEEN, MOVE_NORMAL);
        }
        
        if (stage == MOVEGEN_QUIET || stage == MOVEGEN_ALL) {
            Bitboard quiet = attacks & ~ctx->enemy_pieces;
            count += movegen_add_moves(moves + count, from, quiet, QUEEN, MOVE_NORMAL);
        }
    }
    
    return count;
}

int movegen_king_moves(MoveGenContext *ctx, ChessMove *moves, MoveGenStage stage) {
    int count = 0;
    int from = ctx->king_square;

    if (from < 0 || from > 63) return 0; // defensive guard

    Bitboard attacks = movegen_king_attacks(from) & ~ctx->our_pieces;

    // Regular king moves
    if (stage == MOVEGEN_CAPTURES || stage == MOVEGEN_ALL) {
        Bitboard captures = attacks & ctx->enemy_pieces;
        count += movegen_add_moves(moves + count, from, captures, KING, MOVE_NORMAL);
    }

    if (stage == MOVEGEN_QUIET || stage == MOVEGEN_ALL) {
        Bitboard quiet = attacks & ~ctx->enemy_pieces;
        count += movegen_add_moves(moves + count, from, quiet, KING, MOVE_NORMAL);

        // Castling moves (only in quiet moves)
        count += generate_castling_moves(ctx, moves + count);
    }

    return count;
}

// Separate function for cleaner castling logic
static int generate_castling_moves(MoveGenContext *ctx, ChessMove *moves) {
    int count = 0;
    ChessBoard *board = ctx->board;
    ChessColor us = ctx->us;
    ChessColor them = ctx->them;
    int king_square = ctx->king_square;
    
    // King must not be in check to castle
    if (is_square_attacked_by(board, king_square, them)) {
        return 0;
    }
    
    if (us == WHITE) {
        // White kingside castling (e1-g1)
        if (can_castle(&board->castling_rights, WHITE_KINGSIDE)) {
            if (is_castling_path_clear_and_safe(ctx, 4, 6, 5, 7, 5)) {
                ChessMove castle = create_move(4, 6, KING, NO_PROMOTION, MOVE_CASTLING);
                castle.is_castling = 1;
                castle.rook_location = 7;
                castle.rook_end_location = 5;
                moves[count++] = castle;
            }
        }
        
        // White queenside castling (e1-c1)
        if (can_castle(&board->castling_rights, WHITE_QUEENSIDE)) {
            if (is_castling_path_clear_and_safe(ctx, 4, 2, 3, 0, 3)) {
                ChessMove castle = create_move(4, 2, KING, NO_PROMOTION, MOVE_CASTLING);
                castle.is_castling = 1;
                castle.rook_location = 0;
                castle.rook_end_location = 3;
                moves[count++] = castle;
            }
        }
    } else { // BLACK
        // Black kingside castling (e8-g8)
        if (can_castle(&board->castling_rights, BLACK_KINGSIDE)) {
            if (is_castling_path_clear_and_safe(ctx, 60, 62, 61, 63, 61)) {
                ChessMove castle = create_move(60, 62, KING, NO_PROMOTION, MOVE_CASTLING);
                castle.is_castling = 1;
                castle.rook_location = 63;
                castle.rook_end_location = 61;
                moves[count++] = castle;
            }
        }
        
        // Black queenside castling (e8-c8)
        if (can_castle(&board->castling_rights, BLACK_QUEENSIDE)) {
            if (is_castling_path_clear_and_safe(ctx, 60, 58, 59, 56, 59)) {
                ChessMove castle = create_move(60, 58, KING, NO_PROMOTION, MOVE_CASTLING);
                castle.is_castling = 1;
                castle.rook_location = 56;
                castle.rook_end_location = 59;
                moves[count++] = castle;
            }
        }
    }
    
    return count;
}

// Helper function to check if castling path is clear and safe
static int is_castling_path_clear_and_safe(MoveGenContext *ctx, int king_from, int king_to, 
                                          int rook_to, int rook_from, int check_square) {
    ChessBoard *board = ctx->board;
    ChessColor them = ctx->them;
    
    // Check if squares between king and rook are empty
    int min_sq = (king_from < rook_from) ? king_from : rook_from;
    int max_sq = (king_from > rook_from) ? king_from : rook_from;
    
    for (int sq = min_sq + 1; sq < max_sq; sq++) {
        if (sq == rook_from) continue; // Skip the rook square itself
        if (ctx->all_pieces & (1ULL << sq)) {
            return 0; // Path blocked
        }
    }
    
    // Check that the king doesn't pass through or land on an attacked square
    int step = (king_to > king_from) ? 1 : -1;
    for (int sq = king_from + step; ; sq += step) {
        if (is_square_attacked_by(board, sq, them)) {
            return 0; // King passes through check
        }
        if (sq == king_to) break;
    }
    
    return 1; // Castling is legal
}


// Main optimized move generation functions
int movegen_generate_all(MoveGenContext *ctx, ChessMove *moves) {
    int count = 0;
    
    count += movegen_pawn_moves(ctx, moves + count, MOVEGEN_ALL);
    count += movegen_knight_moves(ctx, moves + count, MOVEGEN_ALL);
    count += movegen_bishop_moves(ctx, moves + count, MOVEGEN_ALL);
    count += movegen_rook_moves(ctx, moves + count, MOVEGEN_ALL);
    count += movegen_queen_moves(ctx, moves + count, MOVEGEN_ALL);
    count += movegen_king_moves(ctx, moves + count, MOVEGEN_ALL);
    
    return count;
}

int movegen_generate_captures(MoveGenContext *ctx, ChessMove *moves) {
    int count = 0;
    
    count += movegen_pawn_moves(ctx, moves + count, MOVEGEN_CAPTURES);
    count += movegen_knight_moves(ctx, moves + count, MOVEGEN_CAPTURES);
    count += movegen_bishop_moves(ctx, moves + count, MOVEGEN_CAPTURES);
    count += movegen_rook_moves(ctx, moves + count, MOVEGEN_CAPTURES);
    count += movegen_queen_moves(ctx, moves + count, MOVEGEN_CAPTURES);
    count += movegen_king_moves(ctx, moves + count, MOVEGEN_CAPTURES);
    
    return count;
}

int movegen_generate_quiet(MoveGenContext *ctx, ChessMove *moves) {
    int count = 0;
    
    count += movegen_pawn_moves(ctx, moves + count, MOVEGEN_QUIET);
    count += movegen_knight_moves(ctx, moves + count, MOVEGEN_QUIET);
    count += movegen_bishop_moves(ctx, moves + count, MOVEGEN_QUIET);
    count += movegen_rook_moves(ctx, moves + count, MOVEGEN_QUIET);
    count += movegen_queen_moves(ctx, moves + count, MOVEGEN_QUIET);
    count += movegen_king_moves(ctx, moves + count, MOVEGEN_QUIET);
    
    return count;
}

// Placeholder implementations for pin/check detection
Bitboard movegen_find_pinned_pieces(MoveGenContext *ctx) {
    // TODO: Implement efficient pin detection
    return 0;
}

Bitboard movegen_find_checkers(MoveGenContext *ctx) {
    // TODO: Implement efficient check detection  
    return 0;
}

int movegen_is_square_attacked(MoveGenContext *ctx, int square, ChessColor by_color) {
    // TODO: Implement efficient square attack detection
    return 0;
}

// Main optimized move generation function
int generate_moves_optimized(ChessBoard *board, ChessMove *moves, MoveGenStage stage) {
    MoveGenContext ctx;
    movegen_init_context(&ctx, board);
    
    int count = 0;
    
    switch (stage) {
        case MOVEGEN_ALL:
            count += movegen_pawn_moves(&ctx, moves + count, stage);
            count += movegen_knight_moves(&ctx, moves + count, stage);
            count += movegen_bishop_moves(&ctx, moves + count, stage);
            count += movegen_rook_moves(&ctx, moves + count, stage);
            count += movegen_queen_moves(&ctx, moves + count, stage);
            count += movegen_king_moves(&ctx, moves + count, stage);
            break;
            
        case MOVEGEN_CAPTURES:
            count = movegen_generate_captures(&ctx, moves);
            break;
            
        case MOVEGEN_QUIET:
            count = movegen_generate_quiet(&ctx, moves);
            break;
            
        case MOVEGEN_CHECKS:
            // TODO: Implement check-only generation
            count = 0;
            break;
    }
    
    return count;
}
