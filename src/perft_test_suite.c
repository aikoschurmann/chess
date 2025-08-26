#include "perft.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    const char* name;
    const char* fen;
    int max_depth;
    uint64_t expected_nodes[10]; // Support up to depth 9
} PerftTestPosition;

// Test positions with expected results
static const PerftTestPosition test_positions[] = {
    {
        "Starting Position",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        7,
        {0, 20, 400, 8902, 197281, 4865609, 119060324, 3195901860, 0, 0}
    },
    {
        "Kiwipete Position",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        6,
        {0, 48, 2039, 97862, 4085603, 193690690, 8031647685ULL, 0, 0, 0}
    },
    {
        "Position 3",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        8,
        {0, 14, 191, 2812, 43238, 674624, 11030083, 178633661, 3009794393ULL, 0}
    },
    {
        "Position 4",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        6,
        {0, 6, 264, 9467, 422333, 15833292, 706045033, 0, 0, 0}
    },
    {
        "Position 5",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        5,
        {0, 44, 1486, 62379, 2103487, 89941194, 0, 0, 0, 0}
    },
    {
        "Position 6",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        6,
        {0, 46, 2079, 89890, 3894594, 164075551, 6923051137, 0, 0, 0}
    }
};

static const int num_test_positions = sizeof(test_positions) / sizeof(test_positions[0]);

// Colors for output
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

static void print_separator(void) {
    printf("================================================================================\n");
}

static void print_header(const char* position_name, const char* fen) {
    print_separator();
    printf(ANSI_COLOR_CYAN "Testing: %s" ANSI_COLOR_RESET "\n", position_name);
    printf("FEN: %s\n", fen);
    print_separator();
}

static void print_result(int depth, uint64_t expected, uint64_t actual, double time_seconds) {
    const char* status_color;
    const char* status_text;
    
    if (expected == 0) {
        // Skip depths we don't have expected results for
        return;
    }
    
    if (actual == expected) {
        status_color = ANSI_COLOR_GREEN;
        status_text = "PASS";
    } else {
        status_color = ANSI_COLOR_RED;
        status_text = "FAIL";
    }
    
    printf("%sDepth %d: %s" ANSI_COLOR_RESET "\n", status_color, depth, status_text);
    printf("  Expected: %20llu\n", (unsigned long long)expected);
    printf("  Actual:   %20llu\n", (unsigned long long)actual);
    
    if (actual != expected) {
        long long diff = (long long)actual - (long long)expected;
        printf("  Diff:     %+20lld\n", diff);
    }
    
    printf("  Time:     %17.3f ms\n", time_seconds * 1000.0);
    printf("\n");
}

static void run_single_position_test(const PerftTestPosition* test_pos) {
    print_header(test_pos->name, test_pos->fen);
    
    ChessBoard board;
    parse_fen(test_pos->fen, &board);
    
    int passed = 0;
    int total = 0;
    
    for (int depth = 1; depth <= test_pos->max_depth; depth++) {
        if (test_pos->expected_nodes[depth] == 0) {
            continue; // Skip depths without expected results
        }
        
        total++;
        
        clock_t start = clock();
        uint64_t actual = perft_public(&board, depth);
        clock_t end = clock();
        
        double time_seconds = (double)(end - start) / CLOCKS_PER_SEC;
        
        print_result(depth, test_pos->expected_nodes[depth], actual, time_seconds);
        
        if (actual == test_pos->expected_nodes[depth]) {
            passed++;
        }
    }
    
    printf(ANSI_COLOR_BLUE "Summary for %s: %d/%d tests passed" ANSI_COLOR_RESET "\n", 
           test_pos->name, passed, total);
    printf("\n");
}

void run_perft_test_suite(void) {
    printf(ANSI_COLOR_MAGENTA "PERFT TEST SUITE" ANSI_COLOR_RESET "\n");
    printf("Running comprehensive perft tests on standard chess positions\n");
    printf("\n");
    
    int total_passed = 0;
    int total_tests = 0;
    
    for (int i = 0; i < num_test_positions; i++) {
        run_single_position_test(&test_positions[i]);
        
        // Count how many tests passed for this position
        for (int depth = 1; depth <= test_positions[i].max_depth; depth++) {
            if (test_positions[i].expected_nodes[depth] != 0) {
                total_tests++;
                
                ChessBoard board;
                parse_fen(test_positions[i].fen, &board);
                uint64_t actual = perft_public(&board, depth);
                
                if (actual == test_positions[i].expected_nodes[depth]) {
                    total_passed++;
                }
            }
        }
    }
    
    print_separator();
    if (total_passed == total_tests) {
        printf(ANSI_COLOR_GREEN "ALL TESTS PASSED! (%d/%d)" ANSI_COLOR_RESET "\n", 
               total_passed, total_tests);
    } else {
        printf(ANSI_COLOR_RED "SOME TESTS FAILED: %d/%d passed" ANSI_COLOR_RESET "\n", 
               total_passed, total_tests);
    }
    print_separator();
}

void run_quick_perft_test_suite(void) {
    printf(ANSI_COLOR_MAGENTA "QUICK PERFT TEST SUITE" ANSI_COLOR_RESET "\n");
    printf("Running quick perft tests (up to depth 4 only)\n");
    printf("\n");
    
    int total_passed = 0;
    int total_tests = 0;
    
    for (int i = 0; i < num_test_positions; i++) {
        print_header(test_positions[i].name, test_positions[i].fen);
        
        ChessBoard board;
        parse_fen(test_positions[i].fen, &board);
        
        int max_test_depth = test_positions[i].max_depth - 1;
        
        for (int depth = 1; depth <= max_test_depth; depth++) {
            if (test_positions[i].expected_nodes[depth] == 0) {
                continue;
            }
            
            total_tests++;
            
            clock_t start = clock();
            uint64_t actual = perft_public(&board, depth);
            clock_t end = clock();
            
            double time_seconds = (double)(end - start) / CLOCKS_PER_SEC;
            
            print_result(depth, test_positions[i].expected_nodes[depth], actual, time_seconds);
            
            if (actual == test_positions[i].expected_nodes[depth]) {
                total_passed++;
            }
        }
        printf("\n");
    }
    
    print_separator();
    if (total_passed == total_tests) {
        printf(ANSI_COLOR_GREEN "ALL QUICK TESTS PASSED! (%d/%d)" ANSI_COLOR_RESET "\n", 
               total_passed, total_tests);
    } else {
        printf(ANSI_COLOR_RED "SOME QUICK TESTS FAILED: %d/%d passed" ANSI_COLOR_RESET "\n", 
               total_passed, total_tests);
    }
    print_separator();
}
