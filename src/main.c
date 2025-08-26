#include "gui.h"
#include "chess_gui.h"
#include "chess_bitboard.h"
#include "timer.h"
#include "debugstate.h"
#include "input.h"
#include "chess_board.h"
#include "move_generation.h"
#include "perft.h"
#include "bot.h"
#include "magic_bitboards.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Forward declarations
static void initialize_engine(void);
static void run_command_line_mode(int argc, char const *argv[]);
static void run_gui_mode(void);
static int handle_command_line_args(int argc, char const *argv[]);
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

static int handle_command_line_args(int argc, char const *argv[]) {
    if (argc <= 1) {
        return 0;  // No command line args, continue to GUI
    }
    
    ChessBoard board;
    initialize_board(&board);
    
    if (strcmp(argv[1], "--perft") == 0 && argc > 2) {
        int depth = atoi(argv[2]);
        run_perft_tests_up_to(depth);
        return 1;  // Command completed
    }
    
    if (strcmp(argv[1], "--divide") == 0 && argc > 2) {
        int depth = atoi(argv[2]);
        uint64_t total = perft_divide(&board, depth);
        return 1;  // Command completed
    }
    
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printf("Chess Engine Usage:\n");
        printf("  %s                    - Run GUI mode\n", argv[0]);
        printf("  %s --perft <depth>    - Run perft tests up to depth\n", argv[0]);
        printf("  %s --divide <depth>   - Run perft divide at depth\n", argv[0]);
        printf("  %s --help             - Show this help\n", argv[0]);
        return 1;  // Command completed
    }
    
    printf("Unknown command line argument: %s\n", argv[1]);
    return 1;  // Command completed with error
}
static void run_gui_mode(void) {
    // Game state
    DebugState debugstate = {PAWN, 1, 0};  // Start with Pawn, white's bitboard, draw bitboards off
    ChessBoard board;
    ChessMove moves[256];
    ChessMove tile_moves[256];
    int num_moves = 0;
    int num_tile_moves = 0;
    unsigned long long moves_mask = 0;
    int selected_tile = -1;
    
    // Initialize GUI and game
    initialize_window("Chess Engine", SCREEN_WIDTH, SCREEN_HEIGHT);
    initialize_keyboard_state();
    initialize_sprites();
    initialize_board(&board);
    initialize_timer();
    
    // Generate initial moves
    generate_moves_fast(&board, moves, &num_moves);
    verify_king_safety(&board, moves, &num_moves);
    
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
            // Regenerate moves for new position
            num_moves = 0;
            generate_moves_fast(&board, moves, &num_moves);
            verify_king_safety(&board, moves, &num_moves);
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
            *selected_tile = -1;
            *num_tile_moves = 0;
            *moves_mask = 0;
            
            // Regenerate moves for the new position and new turn
            *num_moves = 0;
            generate_moves_fast(board, moves, num_moves);
            verify_king_safety(board, moves, num_moves);
            
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
    
    // Find all moves from this tile
    for (int i = 0; i < num_moves; i++) {
        if (moves[i].start_tile == tile) {
            tile_moves[(*num_tile_moves)++] = moves[i];
        }
    }
    
    if (*num_tile_moves > 0) {
        *selected_tile = tile;
    } else {
        *selected_tile = -1;
    }
    
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
