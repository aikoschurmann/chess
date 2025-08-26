#ifndef CHESS_GUI_H
#define CHESS_GUI_H

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600
#define PADDING 50
#define UI_PANEL_WIDTH 300  // Width of the UI panel when visible
#define MIN_BOARD_SIZE 400  // Minimum board size
#define MAX_BOARD_SIZE 560  // Maximum board size

#include "gui.h"
#include "chess_bitboard.h"
#include "chess_board.h"
#include <pthread.h>

SDL_Texture *white_pawn_image;
SDL_Texture *black_pawn_image;
SDL_Texture *white_knight_image;
SDL_Texture *black_knight_image;
SDL_Texture *white_bishop_image;
SDL_Texture *black_bishop_image;
SDL_Texture *white_rook_image;
SDL_Texture *black_rook_image;
SDL_Texture *white_queen_image;
SDL_Texture *black_queen_image;
SDL_Texture *white_king_image;
SDL_Texture *black_king_image;

SDL_Texture *white_sprites[6]; 
SDL_Texture *black_sprites[6];

// Game state tracking
typedef struct {
    int move_count;
    int half_move_clock;
    ChessColor current_player;
    int is_in_check;
    int is_checkmate;
    int is_stalemate;
    int show_coordinates;
    int show_ui_panel;  // Toggle for UI panel visibility
    int show_move_hints;  // Toggle for move possibilities highlighting
    int selected_square;
    int last_move_from;
    int last_move_to;
    // Search progress (for background engine runs)
    int is_searching;              // non-zero while a background search is running
    int search_done;               // becomes non-zero when a background search finished with a move
    ChessMove search_best_move;    // latest best move reported by the search
    int search_depth;              // current search depth being reported
    int search_score;              // current score reported by search
    unsigned int search_elapsed_ms; // elapsed ms since search started (updated by callback)
    pthread_mutex_t search_lock;   // protect the search_* fields
    // Dynamic move history arrays
    char **move_history;     // Dynamic array of move strings
    ChessBoard *board_history;  // Dynamic array of board positions
    int move_history_capacity;  // Current capacity of the arrays
    
    // Move history navigation
    int viewing_move_index;  // Which move we're currently viewing (-1 = current position)
    int actual_move_count;   // The actual number of moves played (vs viewing position)
    int move_history_scroll_offset;  // For scrolling through move history
} GameUIState;

// Dynamic layout calculation functions
int get_board_size(int panel_visible);
int get_square_size(int panel_visible);
int get_board_x_offset(int panel_visible);
int get_board_y_offset(int panel_visible);
int get_ui_panel_x_offset(int panel_visible);
int get_available_board_width(int panel_visible);

void draw_chess_board();
void draw_chess_board_dynamic(GameUIState *ui_state);
void draw_bitboard_mask(Bitboard bitboard, int r, int g, int b, int a);
void draw_bitboard_mask_dynamic(Bitboard bitboard, int r, int g, int b, int a, GameUIState *ui_state);
void draw_bitboard_mask_adaptive(Bitboard bitboard, GameUIState *ui_state);
void draw_pieces(ChessBoard *board);
void draw_pieces_dynamic(ChessBoard *board, GameUIState *ui_state);
void draw_ui_panel(GameUIState *ui_state);
void draw_board_coordinates();
void draw_board_coordinates_dynamic(GameUIState *ui_state);
void draw_move_history(const GameUIState *ui_state);
void draw_game_status(const GameUIState *ui_state);
void highlight_last_move(GameUIState *ui_state);
void highlight_selected_square(GameUIState *ui_state);
void update_game_ui_state(GameUIState *ui_state, ChessBoard *board, int num_moves, int selected_tile);

// Utility
int calculate_visible_rows(void);

// Move history navigation
int handle_move_history_click(int mouse_x, int mouse_y, GameUIState *ui_state);
void navigate_to_move(GameUIState *ui_state, int move_index);
void save_board_position(GameUIState *ui_state, ChessBoard *board);
ChessBoard* get_viewing_board(GameUIState *ui_state, ChessBoard *current_board);

// Dynamic move history management
void init_move_history(GameUIState *ui_state);
void cleanup_move_history(GameUIState *ui_state);
void expand_move_history(GameUIState *ui_state);
void handle_move_history_keyboard(GameUIState *ui_state, int key);
void scroll_move_history(GameUIState *ui_state, int direction);
void ensure_move_visible(GameUIState *ui_state);

void chess_coordinates_to_screen(int *x, int *y);
void screen_to_chess_coordinates(int *x, int *y);
void chess_coordinates_to_screen_dynamic(int *x, int *y, GameUIState *ui_state);
void screen_to_chess_coordinates_dynamic(int *x, int *y, GameUIState *ui_state);
void initialize_sprites();

#endif