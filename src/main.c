#include "gui.h"
#include "chess_gui.h"
#include "chess_bitboard.h"
#include "timer.h"
#include "debugstate.h"
#include "input.h"
#include "chess_board.h"
#include "move_generation.h"
#include "perft.h"
#include "perft_test_suite.h"
#include "bot.h"
#include "magic_bitboards.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Constants
#define MAX_MOVES 256
#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

// Command line argument structure
typedef struct {
    const char *command;
    int min_args;
    const char *description;
} CommandInfo;

// Forward declarations
static void initialize_engine(void);
static void run_gui_mode(void);
static int handle_command_line_args(int argc, char const *argv[]);
static const char *parse_fen_argument(int argc, char const *argv[]);
static void print_help(const char *program_name);
static void handle_mouse_click(ChessBoard *board, ChessMove *moves, int *num_moves,
                              int *selected_tile, ChessMove *tile_moves, 
                              int *num_tile_moves, unsigned long long *moves_mask);
static int try_make_move(ChessBoard *board, ChessMove *moves, int num_moves, 
                        int from_tile, int to_tile);
static void select_piece(ChessBoard *board, ChessMove *moves, int num_moves, int tile,
                        int *selected_tile, ChessMove *tile_moves, 
                        int *num_tile_moves, unsigned long long *moves_mask);
static void render_game(ChessBoard *board, DebugState *debugstate, 
                       unsigned long long moves_mask);
static void regenerate_moves(ChessBoard *board, ChessMove *moves, int *num_moves);

int main(int argc, char const *argv[]) {
    // Initialize core engine components
    initialize_engine();
    
    // Check if we should run in command line mode
    if (handle_command_line_args(argc, argv)) {
        return 0;  // Command line mode completed
    }
    
    // Run GUI mode
    run_gui_mode();
    
    return 0;
}

static void initialize_engine(void) {
    initialize_bitboards();
    initialize_magic_bitboards();
}

static const char *parse_fen_argument(int argc, char const *argv[]) {
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--fen") == 0) {
            return argv[i + 1];
        }
    }
    return DEFAULT_FEN;
}

static void print_help(const char *program_name) {
    printf("Chess Engine Usage:\n");
    printf("  %-35s - Run GUI mode\n", program_name);
    printf("  %-35s - Run perft tests up to depth\n", "program --perft <depth> [--fen \"...\"]");
    printf("  %-35s - Run perft divide at depth\n", "program --divide <depth> [--fen \"...\"]");
    printf("  %-35s - Run comprehensive perft test suite\n", "program --test-suite");
    printf("  %-35s - Run quick perft test suite (depth ≤ 4)\n", "program --quick-test");
    printf("  %-35s - Parse and validate FEN string\n", "program --fen \"<fen_string>\"");
    printf("  %-35s - Show this help\n", "program --help");
    printf("\nExamples:\n");
    printf("  %s --divide 4 --fen \"%s\"\n", program_name, DEFAULT_FEN);
    printf("  %s --perft 5 --fen \"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1\"\n", program_name);
    printf("  %s --test-suite\n", program_name);
    printf("  %s --quick-test\n", program_name);
}

static int handle_command_line_args(int argc, char const *argv[]) {
    if (argc <= 1) {
        return 0;  // No command line args, continue to GUI
    }

    const char *command = argv[1];
    const char *fen_to_use = parse_fen_argument(argc, argv);
    ChessBoard board;
    parse_fen(fen_to_use, &board);

    // Handle perft command
    if (strcmp(command, "--perft") == 0 && argc > 2) {
        int depth = atoi(argv[2]);
        
        if (strcmp(fen_to_use, DEFAULT_FEN) == 0) {
            run_perft_tests_up_to(depth);
        } else {
            printf("Position: %s\n", fen_to_use);
            run_perft_on_position(&board, depth);
        }
        return 1;
    }

    // Handle divide command
    if (strcmp(command, "--divide") == 0 && argc > 2) {
        int depth = atoi(argv[2]);
        printf("Position: %s\n", fen_to_use);
        perft_divide(&board, depth);
        return 1;
    }

    // Handle test suite commands
    if (strcmp(command, "--test-suite") == 0) {
        run_perft_test_suite();
        return 1;
    }

    if (strcmp(command, "--quick-test") == 0) {
        run_quick_perft_test_suite();
        return 1;
    }

    // Handle FEN validation
    if (strcmp(command, "--fen") == 0 && argc > 2) {
        printf("FEN parsed successfully: %s\n", argv[2]);
        printf("Board state loaded.\n");
        return 1;
    }

    // Handle help
    if (strcmp(command, "--help") == 0 || strcmp(command, "-h") == 0) {
        print_help(argv[0]);
        return 1;
    }

    // Unknown command
    printf("Unknown command: %s\n", command);
    printf("Use %s --help for usage information.\n", argv[0]);
    return 1;
}
static void regenerate_moves(ChessBoard *board, ChessMove *moves, int *num_moves) {
    *num_moves = 0;
    generate_moves_fast(board, moves, num_moves);
    verify_king_safety(board, moves, num_moves);
}

static void run_gui_mode(void) {
    // Game state
    DebugState debugstate = {PAWN, 1, 0};
    ChessBoard board;
    ChessMove moves[MAX_MOVES];
    ChessMove tile_moves[MAX_MOVES];
    int num_moves = 0;
    int num_tile_moves = 0;
    unsigned long long moves_mask = 0;
    int selected_tile = -1;
    
    // Initialize GUI and game
    initialize_window("Chess Engine", SCREEN_WIDTH, SCREEN_HEIGHT);
    initialize_keyboard_state();
    initialize_sprites();
    parse_fen(DEFAULT_FEN, &board);
    initialize_timer();
    
    // Generate initial moves
    regenerate_moves(&board, moves, &num_moves);
    
    // Main game loop
    while (should_continue) {
        handle_events();
        handle_input(&debugstate);
        
        // Handle board reset
        if (is_key_down(SDL_SCANCODE_R)) {
            initialize_board(&board);
            selected_tile = -1;
            num_tile_moves = 0;
            moves_mask = 0;
            regenerate_moves(&board, moves, &num_moves);
        }
        
        // Handle mouse clicks for piece movement
        if (mouse_clicked == 1) {
            handle_mouse_click(&board, moves, &num_moves, &selected_tile, 
                             tile_moves, &num_tile_moves, &moves_mask);
        }
        
        // Render everything
        render_game(&board, &debugstate, moves_mask);
    }
}

static void handle_mouse_click(ChessBoard *board, ChessMove *moves, int *num_moves,
                              int *selected_tile, ChessMove *tile_moves, 
                              int *num_tile_moves, unsigned long long *moves_mask) {
    int x = mouse_location[0];
    int y = mouse_location[1];
    
    screen_to_chess_coordinates(&x, &y);
    int tile = y * 8 + x;
    
    // Check if this is a move attempt (second click)
    if (*selected_tile != -1 && *selected_tile != tile) {
        if (try_make_move(board, moves, *num_moves, *selected_tile, tile)) {
            // Move was successful - reset selection and regenerate moves
            *selected_tile = -1;
            *num_tile_moves = 0;
            *moves_mask = 0;
            regenerate_moves(board, moves, num_moves);
            return;
        }
    }
    
    // Handle piece selection
    select_piece(board, moves, *num_moves, tile, selected_tile, 
                tile_moves, num_tile_moves, moves_mask);
}

static int try_make_move(ChessBoard *board, ChessMove *moves, int num_moves, 
                        int from_tile, int to_tile) {
    // Find and execute the move if it exists
    for (int i = 0; i < num_moves; i++) {
        if (moves[i].start_tile == from_tile && moves[i].end_tile == to_tile) {
            apply_move_simple(board, &moves[i]);
            return 1;  // Move successful
        }
    }
    return 0;  // No valid move found
}

static void select_piece(ChessBoard *board, ChessMove *moves, int num_moves, int tile,
                        int *selected_tile, ChessMove *tile_moves, 
                        int *num_tile_moves, unsigned long long *moves_mask) {
    *num_tile_moves = 0;
    
    // Find all legal moves from this tile
    for (int i = 0; i < num_moves; i++) {
        if (moves[i].start_tile == tile) {
            tile_moves[(*num_tile_moves)++] = moves[i];
        }
    }
    
    // Update selection state
    *selected_tile = (*num_tile_moves > 0) ? tile : -1;
    
    // Generate visual highlight mask for possible moves
    generate_bitboard_from_moves(tile_moves, *num_tile_moves, moves_mask);
}

static void render_game(ChessBoard *board, DebugState *debugstate, 
                       unsigned long long moves_mask) {
    clear_window();
    
    draw_chess_board();
    draw_pieces(board);
    draw_selected_bitboard(debugstate, board);
    draw_bitboard_mask(moves_mask, 0, 255, 0, 100);
    
    present_window();
}
