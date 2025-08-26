#include "search.h"
#include "eval.h"
#include "move_generation.h"
#include <limits.h>
#include <string.h>
// piece_value is defined in eval.c; declare it here for MVV-LVA ordering
extern int piece_value(int piece_type);

// Quiescence search: only captures to avoid horizon effect
static int quiescence_search(ChessBoard *board, int alpha, int beta) {
    int stand_pat = evaluate_material(board);
    if (stand_pat >= beta) return beta;
    if (alpha < stand_pat) alpha = stand_pat;

    ChessMove moves[256];
    int num_moves = 0;
    generate_moves_fast(board, moves, &num_moves);
    // Filter out moves that leave king in check
    verify_king_safety(board, moves, &num_moves);

    // Filter only captures (including en-passant)
    int cap_count = 0;
    for (int i = 0; i < num_moves; ++i) {
        int is_capture = 0;
        if (moves[i].move_type == MOVE_EN_PASSANT) is_capture = 1;
        else {
            ChessColor them = (board->current_turn == WHITE) ? BLACK : WHITE;
            Bitboard to_bb = 1ULL << moves[i].end_tile;
            if (board->combined[them] & to_bb) is_capture = 1;
        }
        if (is_capture) moves[cap_count++] = moves[i];
    }

    // If no captures, return static eval
    if (cap_count == 0) return stand_pat;

    // Order captures by MVV-LVA (victim value - attacker value)
    int scores[256];
    for (int i = 0; i < cap_count; ++i) {
        int score = 0;
        // victim value
        ChessColor them = (board->current_turn == WHITE) ? BLACK : WHITE;
        int victim = -1;
        Bitboard to_bb = 1ULL << moves[i].end_tile;
        for (int p = 0; p < 6; ++p) {
            if (board->pieces[them][p] & to_bb) { victim = p; break; }
        }
        int victim_val = (victim >= 0) ? piece_value(victim) : 0;
    int attacker_val = piece_value(moves[i].piece_type);
    score = victim_val * 100 - attacker_val;
    // Prefer captures by non-pawn attackers slightly and central captures
    if (moves[i].piece_type != PAWN) score += 100;
    if (moves[i].end_tile >= 27 && moves[i].end_tile <= 36) score += 50;
        scores[i] = score;
    }

    // simple selection sort by score desc
    for (int i = 0; i < cap_count; ++i) {
        int best = i;
        for (int j = i + 1; j < cap_count; ++j) if (scores[j] > scores[best]) best = j;
        if (best != i) {
            ChessMove tm = moves[i]; moves[i] = moves[best]; moves[best] = tm;
            int ts = scores[i]; scores[i] = scores[best]; scores[best] = ts;
        }
    }

    for (int i = 0; i < cap_count; ++i) {
        ChessBoard copy = *board;
        apply_move_simple(&copy, &moves[i]);
        int score = -quiescence_search(&copy, -beta, -alpha);
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

// Negamax alpha-beta implementation. Returns score from perspective of side to move.
int search_alpha_beta(ChessBoard *board, int depth, int alpha, int beta) {
    if (depth <= 0) return quiescence_search(board, alpha, beta);

    ChessMove moves[256];
    int num_moves = 0;
    generate_moves_fast(board, moves, &num_moves);
    // Filter out moves that leave king in check
    verify_king_safety(board, moves, &num_moves);

    if (num_moves == 0) {
        // No moves: use evaluate_material (could detect mate/stalemate here)
        return evaluate_material(board);
    }

    // Move ordering: score captures higher using MVV-LVA and promotions
    int scores[256];
    ChessColor us = board->current_turn;
    ChessColor them = (us == WHITE) ? BLACK : WHITE;
    for (int i = 0; i < num_moves; ++i) {
        int score = 0;
        // Promotion bonus
        if (moves[i].move_type == MOVE_PROMOTION) score += 8000;
        // Captures: victim value * 100 - attacker value
        int is_capture = 0;
        if (moves[i].move_type == MOVE_EN_PASSANT) is_capture = 1;
        else {
            Bitboard to_bb = 1ULL << moves[i].end_tile;
            if (board->combined[them] & to_bb) is_capture = 1;
        }
        if (is_capture) {
            int victim = -1;
            Bitboard to_bb = 1ULL << moves[i].end_tile;
            for (int p = 0; p < 6; ++p) if (board->pieces[them][p] & to_bb) { victim = p; break; }
            int victim_val = (victim >= 0) ? piece_value(victim) : 0;
            int attacker_val = piece_value(moves[i].piece_type);
            score += victim_val * 100 - attacker_val;
            score += 5000; // favor captures overall
        }
    // Slight heuristic: prefer piece moves (not pawns) and central destinations
    if (moves[i].piece_type != PAWN) score += 100;
    if (moves[i].end_tile >= 27 && moves[i].end_tile <= 36) score += 50;
        scores[i] = score;
    }

    // simple selection sort by score descending
    for (int i = 0; i < num_moves; ++i) {
        int best = i;
        for (int j = i + 1; j < num_moves; ++j) if (scores[j] > scores[best]) best = j;
        if (best != i) {
            ChessMove tm = moves[i]; moves[i] = moves[best]; moves[best] = tm;
            int ts = scores[i]; scores[i] = scores[best]; scores[best] = ts;
        }
    }

    int best = INT_MIN + 1;
    for (int i = 0; i < num_moves; ++i) {
        ChessBoard copy = *board;
        apply_move_simple(&copy, &moves[i]);
        int score = -search_alpha_beta(&copy, depth - 1, -beta, -alpha);
        if (score > best) best = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break; // beta cutoff
    }
    return best;
}

int search_best_move(ChessBoard *board, ChessMove *out_move, int depth) {
    ChessMove moves[256];
    int num_moves = 0;
    generate_moves_fast(board, moves, &num_moves);
    // Filter out moves that leave king in check
    verify_king_safety(board, moves, &num_moves);
    if (num_moves == 0) return evaluate_material(board);

    int best_score = INT_MIN + 1;
    int best_idx = -1;
    int alpha = INT_MIN + 1;
    int beta = INT_MAX - 1;

    for (int i = 0; i < num_moves; ++i) {
        ChessBoard copy = *board;
        apply_move_simple(&copy, &moves[i]);
        int score = -search_alpha_beta(&copy, depth - 1, -beta, -alpha);
        if (score > best_score) {
            best_score = score;
            best_idx = i;
        }
        if (score > alpha) alpha = score;
    }

    if (best_idx >= 0 && out_move) *out_move = moves[best_idx];
    return best_score;
}
