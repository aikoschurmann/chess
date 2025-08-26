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
#include "engine.h"
#include "magic_bitboards.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

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
                              int *num_tile_moves, unsigned long long *moves_mask,
                              GameUIState *ui_state);
static int try_make_move(ChessBoard *board, ChessMove *moves, int num_moves, 
                        int from_tile, int to_tile, GameUIState *ui_state);
static void select_piece(ChessBoard *board, ChessMove *moves, int num_moves, int tile,
                        int *selected_tile, ChessMove *tile_moves, 
                        int *num_tile_moves, unsigned long long *moves_mask);
static void render_game(ChessBoard *board, DebugState *debugstate, 
                       unsigned long long moves_mask, GameUIState *ui_state);
static void regenerate_moves(ChessBoard *board, ChessMove *moves, int *num_moves);
static void update_game_state(ChessBoard *board, ChessMove *moves, int num_moves, GameUIState *ui_state);

// Helper: map SDL scancode to SDLK-like identifier used by handlers
static int scancode_to_sdlk(int sc) {
    switch (sc) {
        case SDL_SCANCODE_UP: return SDLK_UP;
        case SDL_SCANCODE_DOWN: return SDLK_DOWN;
        case SDL_SCANCODE_HOME: return SDLK_HOME;
        case SDL_SCANCODE_END: return SDLK_END;
        case SDL_SCANCODE_PAGEUP: return SDLK_PAGEUP;
        case SDL_SCANCODE_PAGEDOWN: return SDLK_PAGEDOWN;
        default: return 0;
    }
}

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

// Thread context for background search
typedef struct { ChessBoard board; GameUIState *ui; struct timespec start_ts; } SearchThreadCtx;

// Progress callback invoked by engine_search_iterative; cb_ctx is SearchThreadCtx*
static void search_progress_cb(const ChessMove *m, int depth, int score, void *cb_ctx) {
    SearchThreadCtx *ctx = (SearchThreadCtx*)cb_ctx;
    GameUIState *ui = ctx->ui;
    pthread_mutex_lock(&ui->search_lock);
    ui->search_depth = depth;
    ui->search_score = score;
    ui->search_best_move = *m;
    struct timespec now_ts;
    clock_gettime(CLOCK_MONOTONIC, &now_ts);
    ui->search_elapsed_ms = (unsigned int)((now_ts.tv_sec - ctx->start_ts.tv_sec) * 1000 + (now_ts.tv_nsec - ctx->start_ts.tv_nsec) / 1000000);
    pthread_mutex_unlock(&ui->search_lock);
}

// Thread entry for background search
static void *search_thread_entry(void *v) {
    SearchThreadCtx *ctx = (SearchThreadCtx*)v;
    ChessMove best = { .start_tile = -1, .end_tile = -1, .move_type = 0 };

    // Mark searching
    pthread_mutex_lock(&ctx->ui->search_lock);
    ctx->ui->is_searching = 1;
    ctx->ui->search_done = 0;
    ctx->ui->search_depth = 0;
    ctx->ui->search_score = 0;
    ctx->ui->search_elapsed_ms = 0;
    pthread_mutex_unlock(&ctx->ui->search_lock);

    // Run iterative search (depth up to 10, 5000ms limit) — give more time for stronger play
    int score = engine_search_iterative(&ctx->board, &best, 7, search_progress_cb, ctx, 5000);

    // Write final result and mark done
    pthread_mutex_lock(&ctx->ui->search_lock);
    ctx->ui->search_best_move = best;
    ctx->ui->search_score = score;
    ctx->ui->is_searching = 0;
    ctx->ui->search_done = 1;
    pthread_mutex_unlock(&ctx->ui->search_lock);

    free(ctx);
    return NULL;
}

static void run_gui_mode(void) {
    // Game state
    DebugState debugstate = {PAWN, 1, 0};
    ChessBoard board;
    ChessMove moves[MAX_MOVES];
    ChessMove tile_moves[MAX_MOVES];
    GameUIState ui_state = {0};  // Initialize UI state
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
    
    // Initialize UI state
    ui_state.current_player = board.current_turn;
    ui_state.show_coordinates = 1;  // Enable coordinates by default
    ui_state.show_ui_panel = 1;     // Show UI panel by default
    ui_state.show_move_hints = 1;   // Show move hints by default
    ui_state.selected_square = -1;
    ui_state.last_move_from = -1;
    ui_state.last_move_to = -1;
    
    // Initialize dynamic move history
    init_move_history(&ui_state);
    ui_state.board_history[0] = board;  // Save initial position
    
    // Generate initial moves
    regenerate_moves(&board, moves, &num_moves);
    
    // Initialize game state
    update_game_state(&board, moves, num_moves, &ui_state);
    
    // Initialize search mutex
    pthread_mutex_init(&ui_state.search_lock, NULL);

    // Main game loop
        while (should_continue) {
        handle_events();
        handle_input(&debugstate);
        
        // Handle keyboard controls
        if (is_key_down(SDL_SCANCODE_R)) {
            initialize_board(&board);
            selected_tile = -1;
            num_tile_moves = 0;
            moves_mask = 0;
            ui_state.selected_square = -1;
            ui_state.last_move_from = -1;
            ui_state.last_move_to = -1;
            ui_state.current_player = board.current_turn;
            ui_state.move_count = 0;
            
            // Reset dynamic move history
            cleanup_move_history(&ui_state);
            init_move_history(&ui_state);
            ui_state.board_history[0] = board;  // Reset board history
            
            regenerate_moves(&board, moves, &num_moves);
            update_game_state(&board, moves, num_moves, &ui_state);
        }
        
        // Toggle UI panel with TAB key
        if (is_key_down(SDL_SCANCODE_TAB)) {
            ui_state.show_ui_panel = !ui_state.show_ui_panel;
        }
        
        // Toggle coordinates with C key
        if (is_key_down(SDL_SCANCODE_C)) {
            ui_state.show_coordinates = !ui_state.show_coordinates;
        }
        
        // Toggle move hints with H key
        if (is_key_down(SDL_SCANCODE_H)) {
            ui_state.show_move_hints = !ui_state.show_move_hints;
        }
        
        // Test checkmate position with M key
        if (is_key_down(SDL_SCANCODE_M)) {
            // Set up a simple checkmate position: King on h8, Queen on g7, King on g6
            const char *checkmate_fen = "7k/6Q1/6K1/8/8/8/8/8 b - - 0 1";
            parse_fen(checkmate_fen, &board);
            selected_tile = -1;
            num_tile_moves = 0;
            moves_mask = 0;
            ui_state.selected_square = -1;
            ui_state.last_move_from = -1;
            ui_state.last_move_to = -1;
            ui_state.current_player = board.current_turn;
            ui_state.move_count = 0;
        }
        
        // Handle move history navigation with arrow keys using key_repeat_should_fire
        const unsigned int REPEAT_INITIAL_DELAY = 300; // ms
        const unsigned int REPEAT_INTERVAL = 80; // ms

        if (key_repeat_should_fire(SDL_SCANCODE_UP, REPEAT_INITIAL_DELAY, REPEAT_INTERVAL)) {
            handle_move_history_keyboard(&ui_state, SDLK_UP);
        }
        if (key_repeat_should_fire(SDL_SCANCODE_DOWN, REPEAT_INITIAL_DELAY, REPEAT_INTERVAL)) {
            handle_move_history_keyboard(&ui_state, SDLK_DOWN);
        }
        if (key_repeat_should_fire(SDL_SCANCODE_HOME, REPEAT_INITIAL_DELAY, REPEAT_INTERVAL)) {
            handle_move_history_keyboard(&ui_state, SDLK_HOME);
        }
        if (key_repeat_should_fire(SDL_SCANCODE_END, REPEAT_INITIAL_DELAY, REPEAT_INTERVAL)) {
            handle_move_history_keyboard(&ui_state, SDLK_END);
        }
        if (key_repeat_should_fire(SDL_SCANCODE_PAGEUP, REPEAT_INITIAL_DELAY, REPEAT_INTERVAL)) {
            handle_move_history_keyboard(&ui_state, SDLK_PAGEUP);
        }
        if (key_repeat_should_fire(SDL_SCANCODE_PAGEDOWN, REPEAT_INITIAL_DELAY, REPEAT_INTERVAL)) {
            handle_move_history_keyboard(&ui_state, SDLK_PAGEDOWN);
        }

        // Mouse wheel scroll for move history
        int wheel = poll_mouse_wheel_delta();
        if (wheel != 0) {
            // Positive wheel -> scroll up (older moves)
            if (wheel > 0) scroll_move_history(&ui_state, -1);
            else scroll_move_history(&ui_state, 1);
        }
        
        // Handle mouse clicks for piece movement
        if (mouse_clicked == 1) {
            handle_mouse_click(&board, moves, &num_moves, &selected_tile, 
                             tile_moves, &num_tile_moves, &moves_mask, &ui_state);
        }
        
        // Update UI state
        ui_state.selected_square = selected_tile;
        
        // Update game state (check, checkmate, stalemate)
        update_game_state(&board, moves, num_moves, &ui_state);
        
        // If it's Black's turn and we're viewing the current position, start a background search
        if (board.current_turn == BLACK && ui_state.viewing_move_index == -1) {
            // If a search finished, apply the move first (avoid spawning new search
            // threads before consuming finished results which can lead to races).
            pthread_mutex_lock(&ui_state.search_lock);
            int finished = ui_state.search_done;
            if (finished) {
                ChessMove bot_move = ui_state.search_best_move;
                ui_state.search_done = 0;
                pthread_mutex_unlock(&ui_state.search_lock);

                // Validate the engine move against current legal moves
                int legal_found = 0;
                ChessMove legal_moves[256]; int legal_count = 0;
                generate_moves_fast(&board, legal_moves, &legal_count);
                verify_king_safety(&board, legal_moves, &legal_count);
                for (int i = 0; i < legal_count; ++i) {
                    if (legal_moves[i].start_tile == bot_move.start_tile && legal_moves[i].end_tile == bot_move.end_tile && legal_moves[i].promotion == bot_move.promotion && legal_moves[i].move_type == bot_move.move_type) { legal_found = 1; break; }
                }

                if (!legal_found) {
                    fprintf(stderr, "[main] background search returned illegal move - ignoring\n");
                } else {
                    // Apply the validated move
                    apply_move_simple(&board, &bot_move);

                    // Update UI state with move information
                    ui_state.last_move_from = bot_move.start_tile;
                    ui_state.last_move_to = bot_move.end_tile;

                    int rows_can_show = calculate_visible_rows();
                    int total_rows_before = (ui_state.actual_move_count + 1) / 2;
                    int max_offset_before = total_rows_before - rows_can_show;
                    if (max_offset_before < 0) max_offset_before = 0;
                    int was_at_bottom = (ui_state.move_history_scroll_offset >= max_offset_before);

                    ui_state.actual_move_count++;
                    ui_state.move_count = ui_state.actual_move_count;
                    ui_state.viewing_move_index = -1;

                    // Save board position
                    save_board_position(&ui_state, &board);

                    if (ui_state.actual_move_count > ui_state.move_history_capacity) {
                        expand_move_history(&ui_state);
                    }

                    if (ui_state.actual_move_count <= ui_state.move_history_capacity) {
                        char move_str[16];
                        sprintf(move_str, "%c%d-%c%d",
                            'a' + (bot_move.start_tile % 8), (bot_move.start_tile / 8) + 1,
                            'a' + (bot_move.end_tile % 8), (bot_move.end_tile / 8) + 1);
                        ui_state.move_history[ui_state.actual_move_count - 1] = malloc(16 * sizeof(char));
                        if (ui_state.move_history[ui_state.actual_move_count - 1]) {
                            strcpy(ui_state.move_history[ui_state.actual_move_count - 1], move_str);
                        }
                    }

                    if (was_at_bottom) {
                        int rows_can_show_new = calculate_visible_rows();
                        int total_rows_new = (ui_state.actual_move_count + 1) / 2;
                        int max_offset_new = total_rows_new - rows_can_show_new;
                        if (max_offset_new < 0) max_offset_new = 0;
                        ui_state.move_history_scroll_offset = max_offset_new;
                    }

                    // Regenerate moves after bot move and update UI
                    regenerate_moves(&board, moves, &num_moves);
                    ui_state.current_player = board.current_turn;
                    update_game_state(&board, moves, num_moves, &ui_state);
                }
            } else {
                pthread_mutex_unlock(&ui_state.search_lock);

                // If not already searching, start one
                pthread_mutex_lock(&ui_state.search_lock);
                int already_searching = ui_state.is_searching;
                pthread_mutex_unlock(&ui_state.search_lock);

                if (!already_searching) {
                    // Reserve search slot immediately under lock to avoid a race
                    // where multiple frames spawn concurrent search threads.
                    pthread_mutex_lock(&ui_state.search_lock);
                    ui_state.is_searching = 1;
                    ui_state.search_done = 0;
                    ui_state.search_depth = 0;
                    ui_state.search_score = 0;
                    ui_state.search_elapsed_ms = 0;
                    pthread_mutex_unlock(&ui_state.search_lock);

                    // Launch background search thread using top-level SearchThreadCtx
                    SearchThreadCtx *ctx = malloc(sizeof(SearchThreadCtx));
                    ctx->board = board;
                    ctx->ui = &ui_state;
                    clock_gettime(CLOCK_MONOTONIC, &ctx->start_ts);

                    pthread_t tid;
                    pthread_create(&tid, NULL, search_thread_entry, ctx);
                    pthread_detach(tid);
                }
            }
        }
        
        // Render everything
        render_game(&board, &debugstate, moves_mask, &ui_state);
    }
    
    // Cleanup dynamic memory before exit
    pthread_mutex_destroy(&ui_state.search_lock);
    cleanup_move_history(&ui_state);
}

static void handle_mouse_click(ChessBoard *board, ChessMove *moves, int *num_moves,
                              int *selected_tile, ChessMove *tile_moves, 
                              int *num_tile_moves, unsigned long long *moves_mask,
                              GameUIState *ui_state) {
    int x = mouse_location[0];
    int y = mouse_location[1];
    
    // First check if click is in move history panel
    if (handle_move_history_click(x, y, ui_state)) {
        // Move history click was handled, clear selection
        *selected_tile = -1;
        *num_tile_moves = 0;
        *moves_mask = 0;
        ui_state->selected_square = -1;
        return;
    }
    
    screen_to_chess_coordinates_dynamic(&x, &y, ui_state);
    int tile = y * 8 + x;
    
    // Check if click is outside the board
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return;  // Ignore clicks outside the board
    }
    
    // Only allow moves when viewing the current position
    if (ui_state->viewing_move_index != -1) {
        return;  // Can't make moves when viewing history
    }
    
    // Check if this is a move attempt (second click)
    if (*selected_tile != -1 && *selected_tile != tile) {
        if (try_make_move(board, moves, *num_moves, *selected_tile, tile, ui_state)) {
            // Move was successful - reset selection and regenerate moves
            *selected_tile = -1;
            *num_tile_moves = 0;
            *moves_mask = 0;
            regenerate_moves(board, moves, num_moves);
            ui_state->current_player = board->current_turn;
            update_game_state(board, moves, *num_moves, ui_state);
            return;
        }
    }
    
    // Handle piece selection
    select_piece(board, moves, *num_moves, tile, selected_tile, 
                tile_moves, num_tile_moves, moves_mask);
}

static int try_make_move(ChessBoard *board, ChessMove *moves, int num_moves, 
                        int from_tile, int to_tile, GameUIState *ui_state) {
    // Find and execute the move if it exists
    for (int i = 0; i < num_moves; i++) {
        if (moves[i].start_tile == from_tile && moves[i].end_tile == to_tile) {
            apply_move_simple(board, &moves[i]);
            
            // Update UI state with move information
            ui_state->last_move_from = from_tile;
            ui_state->last_move_to = to_tile;

            // Determine whether the move history view was already at the bottom.
            // If so, we'll auto-follow new moves; otherwise preserve user's offset.
            int rows_can_show = calculate_visible_rows();
            int total_rows_before = (ui_state->actual_move_count + 1) / 2;
            int max_offset_before = total_rows_before - rows_can_show;
            if (max_offset_before < 0) max_offset_before = 0;
            int was_at_bottom = (ui_state->move_history_scroll_offset >= max_offset_before);

            ui_state->actual_move_count++;
            ui_state->move_count = ui_state->actual_move_count;
            ui_state->viewing_move_index = -1;  // Reset to current position
            
            // Save board position to history
            save_board_position(ui_state, board);
            
            // Add move to history - expand if needed
            if (ui_state->actual_move_count > ui_state->move_history_capacity) {
                expand_move_history(ui_state);
            }
            
            if (ui_state->actual_move_count <= ui_state->move_history_capacity) {
                char move_str[16];
                // Format move using board indexing where rank 1 is at bottom
                sprintf(move_str, "%c%d-%c%d", 
                    'a' + (from_tile % 8), (from_tile / 8) + 1,
                    'a' + (to_tile % 8), (to_tile / 8) + 1);
                
                // Allocate memory for the move string
                ui_state->move_history[ui_state->actual_move_count - 1] = malloc(16 * sizeof(char));
                if (ui_state->move_history[ui_state->actual_move_count - 1]) {
                    strcpy(ui_state->move_history[ui_state->actual_move_count - 1], move_str);
                }
            }

            // If the user was already viewing the bottom, update the scroll
            // offset to follow the newly added move. This keeps the UI in
            // "follow" mode only when the user hasn't manually scrolled up.
            if (was_at_bottom) {
                int rows_can_show_new = calculate_visible_rows();
                int total_rows_new = (ui_state->actual_move_count + 1) / 2;
                int max_offset_new = total_rows_new - rows_can_show_new;
                if (max_offset_new < 0) max_offset_new = 0;
                ui_state->move_history_scroll_offset = max_offset_new;
            }
            
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
                       unsigned long long moves_mask, GameUIState *ui_state) {
    clear_window();
    
    // Get the board we should be viewing (current or historical)
    ChessBoard *viewing_board = get_viewing_board(ui_state, board);
    
    draw_chess_board_dynamic(ui_state);
    
    // Draw highlights before pieces
    highlight_last_move(ui_state);
    highlight_selected_square(ui_state);
    
    // Only draw move hints if enabled and viewing current position
    if (ui_state->show_move_hints && ui_state->viewing_move_index == -1) {
        draw_bitboard_mask_adaptive(moves_mask, ui_state);
    }
    
    draw_pieces_dynamic(viewing_board, ui_state);
    draw_selected_bitboard(debugstate, viewing_board, ui_state);
    
    // Draw coordinates if enabled
    draw_board_coordinates_dynamic(ui_state);
    
    // Draw the UI panel (will handle visibility internally)
    draw_ui_panel(ui_state);
    
    present_window();
}

static void update_game_state(ChessBoard *board, ChessMove *moves, int num_moves, GameUIState *ui_state) {
    // Find the king position
    Bitboard king_bb = board->pieces[board->current_turn][KING];
    if (!king_bb) {
        // No king found - shouldn't happen
        ui_state->is_in_check = 0;
        ui_state->is_checkmate = 0;
        ui_state->is_stalemate = 0;
        return;
    }
    
    int king_square = __builtin_ctzll(king_bb);
    ChessColor opponent = (board->current_turn == WHITE) ? BLACK : WHITE;
    
    // Check if king is in check
    ui_state->is_in_check = is_square_attacked_by(board, king_square, opponent);
    
    // Determine checkmate or stalemate based on available moves
    if (num_moves == 0) {
        if (ui_state->is_in_check) {
            ui_state->is_checkmate = 1;
            ui_state->is_stalemate = 0;
        } else {
            ui_state->is_checkmate = 0;
            ui_state->is_stalemate = 1;
        }
    } else {
        ui_state->is_checkmate = 0;
        ui_state->is_stalemate = 0;
    }
}
