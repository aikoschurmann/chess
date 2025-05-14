#include "bot.h"
#include "move_generation.h"
#include <stdlib.h>
#include <time.h>

void bot_select_move(ChessBoard *board, ChessMove *out_move) {
    ChessMove moves[256];
    int num_moves = 0;

    generate_moves(board, moves, &num_moves);
    verify_king_safety(board, moves, &num_moves);

    if (num_moves == 0) {
        out_move->start_tile = -1;
        out_move->end_tile = -1;
        return;
    }

    srand((unsigned int)time(NULL));
    int choice = rand() % num_moves;
    *out_move = moves[choice];
}

void bot_play(ChessBoard *board) {
    ChessMove move;
    bot_select_move(board, &move);
    if (move.start_tile != -1) {
        apply_move(board, &move);
        return;
    }
    initialize_board(board);
}