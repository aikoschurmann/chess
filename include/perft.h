#ifndef __PERFT_H__
#define __PERFT_H__
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "chess_board.h"
#include "move_generation.h"
#include "chess_bitboard.h"

// ANSI color codes
#define GREEN   "\033[1;32m"
#define RED     "\033[1;31m"
#define YELLOW  "\033[1;33m"
#define RESET   "\033[0m"

// Known correct perft results from starting position
typedef struct {
    int depth;
    uint64_t expected_nodes;
} PerftTest;

static const PerftTest perft_tests[] = {
    {1, 20},
    {2, 400},
    {3, 8902},
    {4, 197281},
    {5, 4865609},
    {6, 119060324},
    {7, 3195901860}
};

static uint64_t perft(const ChessBoard *board, int depth);
static void format_move(const ChessMove *move, char *out, size_t size);
uint64_t perft_divide(const ChessBoard *board, int depth);
void run_perft_tests_up_to(int max_depth);

#endif // __PERFT_H__