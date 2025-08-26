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
    char move_history[100][16];  // Store up to 100 moves as strings
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
void chess_coordinates_to_screen(int *x, int *y);
void screen_to_chess_coordinates(int *x, int *y);
void chess_coordinates_to_screen_dynamic(int *x, int *y, GameUIState *ui_state);
void screen_to_chess_coordinates_dynamic(int *x, int *y, GameUIState *ui_state);
void initialize_sprites();

#endif