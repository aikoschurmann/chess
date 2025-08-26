#include "perft.h"


#define MAX_MOVES 256

static uint64_t perft(const ChessBoard *board, int depth) {
    ChessMove moves[MAX_MOVES];
    int num_moves = 0;
    generate_moves_legacy((ChessBoard*)board, moves, &num_moves);
    verify_king_safety((ChessBoard*)board, moves, &num_moves);

    if (depth == 1) {
        return num_moves;
    }

    uint64_t total_nodes = 0;
    for (int i = 0; i < num_moves; i++) {
        ChessBoard temp_board = *board;
        apply_move(&temp_board, &moves[i]);
        uint64_t nodes = perft(&temp_board, depth - 1);
        total_nodes += nodes;
    }

    return total_nodes;
}

static void format_move(const ChessMove *move, char *out, size_t size) {
    if (size < 6) { if (size > 0) out[0] = '\0'; return; }

    int from = move->start_tile;
    int to   = move->end_tile;

    out[0] = 'a' + (from % 8);
    out[1] = '1' + (from / 8);
    out[2] = 'a' + (to % 8);
    out[3] = '1' + (to / 8);

    if (move->promotion) {
        out[4] = "nbrq"[move->promotion - 2]; // Assuming KNIGHT=2,...,QUEEN=5
        out[5] = '\0';
    } else {
        out[4] = '\0';
    }
}

uint64_t perft_divide(const ChessBoard *board, int depth) {
    ChessMove moves[MAX_MOVES];
    int num_moves = 0;
    generate_moves_legacy((ChessBoard*)board, moves, &num_moves);
    verify_king_safety((ChessBoard*)board, moves, &num_moves);

    uint64_t total = 0;
    printf("  Move     Nodes\n");
    printf("  ----------------\n");

    for (int i = 0; i < num_moves; ++i) {
        ChessBoard copy = *board;
        apply_move(&copy, &moves[i]);
        uint64_t count = perft(&copy, depth - 1);

        char move_str[8];
        format_move(&moves[i], move_str, sizeof(move_str));
        printf("  %-6s : %llu\n", move_str, (unsigned long long)count);
        total += count;
    }

    printf("  ----------------\n");
    printf("  Total   : %llu\n", (unsigned long long)total);
    return total;
}

static double elapsed_seconds(clock_t start) {
    return (double)(clock() - start) / CLOCKS_PER_SEC;
}

static const char* format_nps(double nps, char *buf, size_t buf_size) {
    if (nps >= 1e9)       snprintf(buf, buf_size, "%.2fG", nps / 1e9);
    else if (nps >= 1e6)  snprintf(buf, buf_size, "%.2fM", nps / 1e6);
    else if (nps >= 1e3)  snprintf(buf, buf_size, "%.2fK", nps / 1e3);
    else                  snprintf(buf, buf_size, "%.0f", nps);
    return buf;
}

void run_perft_tests_up_to(int max_depth) {
    ChessBoard board;
    initialize_board(&board); // Assumes this sets up the starting position

    printf("%5s | %15s | %8s | %10s | %10s\n", "Depth", "Expected", "Time", "Nodes/s", "got");
    printf("------+-----------------+----------+------------+------------\n");

    for (size_t i = 0; i < sizeof(perft_tests) / sizeof(perft_tests[0]); ++i) {
        int depth = perft_tests[i].depth;
        if (depth > max_depth) break;

        clock_t start = clock();
        uint64_t nodes = perft(&board, depth);
        double time = elapsed_seconds(start);
        char nps_buf[32];
        format_nps(nodes / time, nps_buf, sizeof(nps_buf));

        const char *color = (nodes == perft_tests[i].expected_nodes) ? GREEN : RED;

        unsigned long long expected = perft_tests[i].expected_nodes;
        printf("%s%5d | %15llu | %6.2fs | %10s | %10llu%s\n",
            color, depth, (unsigned long long)expected, time, nps_buf, (unsigned long long)nodes, RESET
        );

    }

}

