#include "chess_gui.h"
#include "gui.h"

// Chess board color palette - inspired by traditional wooden boards
static const int LIGHT_SQUARE_COLOR[4] = {240, 217, 181, 255};  // Cream/buff color
static const int DARK_SQUARE_COLOR[4] = {181, 136, 99, 255};    // Warm brown color
static const int UI_HIGHLIGHT_COLOR_DARK[4] =   {180, 150, 0, 100};     // Dark brown for move hints
static const int UI_HIGHLIGHT_COLOR_LIGHT[4] = {255, 220, 50, 100}; // Light brown for move hints

// Selection colors - different for light and dark squares for optimal contrast
static const int LIGHT_SQUARE_SELECTION[4] = {150, 180, 200, 255};  // Darker than light squares
static const int DARK_SQUARE_SELECTION[4] = {120, 150, 160, 255};     // Darker than dark squares

// Dynamic layout calculation functions
int get_available_board_width(int panel_visible) {
    if (panel_visible) {
        return SCREEN_WIDTH - UI_PANEL_WIDTH - 2 * PADDING;  // Space for board with panel
    } else {
        return SCREEN_WIDTH - 2 * PADDING;  // Full width minus padding
    }
}

int get_board_size(int panel_visible) {
    int available_width = get_available_board_width(panel_visible);
    int available_height = SCREEN_HEIGHT - 2 * PADDING;
    
    // Use the smaller dimension to ensure the board fits
    int max_size = (available_width < available_height) ? available_width : available_height;
    
    // Clamp to our min/max bounds
    if (max_size < MIN_BOARD_SIZE) max_size = MIN_BOARD_SIZE;
    if (max_size > MAX_BOARD_SIZE) max_size = MAX_BOARD_SIZE;
    
    // Make sure it's divisible by 8 for clean squares
    return (max_size / 8) * 8;
}

int get_square_size(int panel_visible) {
    return get_board_size(panel_visible) / 8;
}

int get_board_x_offset(int panel_visible) {
    int board_size = get_board_size(panel_visible);
    int available_width = get_available_board_width(panel_visible);
    // Center the board in the available space (left side only, excluding panel)
    return PADDING + (available_width - board_size) / 2;
}

int get_board_y_offset(int panel_visible) {
    int board_size = get_board_size(panel_visible);
    int available_height = SCREEN_HEIGHT - 2 * PADDING;
    // Center the board vertically
    return PADDING + (available_height - board_size) / 2;
}

int get_ui_panel_x_offset(int panel_visible) {
    return panel_visible ? (SCREEN_WIDTH - UI_PANEL_WIDTH) : SCREEN_WIDTH;
}


void initialize_sprites() {
    white_pawn_image = load_sprite("../images/pawn_white.png");
    black_pawn_image = load_sprite("../images/pawn_black.png");
    white_knight_image = load_sprite("../images/knight_white.png");
    black_knight_image = load_sprite("../images/knight_black.png");
    white_bishop_image = load_sprite("../images/bishop_white.png");
    black_bishop_image = load_sprite("../images/bishop_black.png");
    white_rook_image = load_sprite("../images/rook_white.png");
    black_rook_image = load_sprite("../images/rook_black.png");
    white_queen_image = load_sprite("../images/queen_white.png");
    black_queen_image = load_sprite("../images/queen_black.png");
    white_king_image = load_sprite("../images/king_white.png");
    black_king_image = load_sprite("../images/king_black.png");

    white_sprites[PAWN] = white_pawn_image;
    white_sprites[KNIGHT] = white_knight_image;
    white_sprites[BISHOP] = white_bishop_image;
    white_sprites[ROOK] = white_rook_image;
    white_sprites[QUEEN] = white_queen_image;
    white_sprites[KING] = white_king_image;

    black_sprites[PAWN] = black_pawn_image;
    black_sprites[KNIGHT] = black_knight_image;
    black_sprites[BISHOP] = black_bishop_image;
    black_sprites[ROOK] = black_rook_image;
    black_sprites[QUEEN] = black_queen_image;
    black_sprites[KING] = black_king_image;
}



void draw_chess_board_dynamic(GameUIState *ui_state) {
    int panel_visible = ui_state->show_ui_panel;
    
    int square_size = get_square_size(panel_visible);
    int board_x = get_board_x_offset(panel_visible);
    int board_y = get_board_y_offset(panel_visible);

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            // Alternate color based on sum of row and column indices
            if ((i + j) % 2 == 0) {
                set_color(LIGHT_SQUARE_COLOR[0], LIGHT_SQUARE_COLOR[1], LIGHT_SQUARE_COLOR[2], LIGHT_SQUARE_COLOR[3]);
            } else {
                set_color(DARK_SQUARE_COLOR[0], DARK_SQUARE_COLOR[1], DARK_SQUARE_COLOR[2], DARK_SQUARE_COLOR[3]);
            }

            draw_filled_rectangle(board_x + j * square_size, board_y + i * square_size, square_size, square_size);
        }
    }
}

void chess_coordinates_to_screen_dynamic(int *x, int *y, GameUIState *ui_state) {
    int panel_visible = ui_state->show_ui_panel;
    
    int square_size = get_square_size(panel_visible);
    int board_x = get_board_x_offset(panel_visible);
    int board_y = get_board_y_offset(panel_visible);
    
    *x = board_x + *x * square_size;
    *y = board_y + (7 - *y) * square_size;  // Flip Y coordinate for screen display
}

void screen_to_chess_coordinates_dynamic(int *x, int *y, GameUIState *ui_state) {
    int panel_visible = ui_state->show_ui_panel;
    
    int square_size = get_square_size(panel_visible);
    int board_x = get_board_x_offset(panel_visible);
    int board_y = get_board_y_offset(panel_visible);
    
    *x = (*x - board_x) / square_size;
    *y = 7 - ((*y - board_y) / square_size);  // Flip Y coordinate back to chess notation
}

void draw_bitboard_mask_dynamic(Bitboard bitboard, int r, int g, int b, int a, GameUIState *ui_state) {
    int square_size = get_square_size(ui_state->show_ui_panel);
    
    for (int i = 0; i < 64; i++) {
        if (bitboard & (1ULL << i)) {
            int x = i % 8;
            int y = i / 8;
            chess_coordinates_to_screen_dynamic(&x, &y, ui_state);
            set_color(r, g, b, a);
            draw_rectangle(x, y, square_size, square_size);
        }
    }
}

void draw_bitboard_mask_adaptive(Bitboard bitboard, GameUIState *ui_state) {
    int square_size = get_square_size(ui_state->show_ui_panel);
    
    for (int i = 0; i < 64; i++) {
        if (bitboard & (1ULL << i)) {
            int chess_x = i % 8;
            int chess_y = i / 8;
            int screen_x = chess_x;
            int screen_y = chess_y;
            chess_coordinates_to_screen_dynamic(&screen_x, &screen_y, ui_state);
            
            // Convert chess coordinates to board drawing coordinates
            int board_row = 7 - chess_y;  // Flip Y to match board drawing
            int board_col = chess_x;
            
            // Match the board drawing logic: (board_row + board_col) % 2 == 0 for light squares
            int *highlight_color = ((board_row + board_col) % 2 == 0) ? UI_HIGHLIGHT_COLOR_LIGHT : UI_HIGHLIGHT_COLOR_DARK;
            set_color(highlight_color[0], highlight_color[1], highlight_color[2], highlight_color[3]);
            draw_filled_rectangle(screen_x, screen_y, square_size, square_size);
        }
    }
}

void draw_pieces_dynamic(ChessBoard *board, GameUIState *ui_state) {
    int square_size = get_square_size(ui_state->show_ui_panel);
    
    // Calculate piece size - leave some padding for better appearance
    int piece_size = (int)(square_size * 0.85f);  // 85% of square size
    int piece_offset = (square_size - piece_size) / 2;  // Center the piece
    
    // Draw pieces for both colors
    for (int color = 0; color < 2; color++) {
        SDL_Texture **sprites = (color == WHITE) ? white_sprites : black_sprites;
        
        for (int piece_type = 0; piece_type < 6; piece_type++) {
            Bitboard current_pieces = board->pieces[color][piece_type];
            
            while (current_pieces) {
                int index = __builtin_ctzll(current_pieces);  // Find the lowest set bit
                int x = index % 8;
                int y = index / 8;
                
                chess_coordinates_to_screen_dynamic(&x, &y, ui_state);  // Convert to screen coordinates
                SDL_Texture *sprite = sprites[piece_type];  // Get the appropriate sprite
                
                // Draw piece centered in square with padding
                draw_sprite_scaled(sprite, x + piece_offset, y + piece_offset, piece_size, piece_size);
                
                current_pieces &= current_pieces - 1;  // Clear the lowest set bit
            }
        }
    }
}

// Replace your current draw_ui_panel with this improved version.
void draw_ui_panel(GameUIState *ui_state) {
    if (!ui_state->show_ui_panel) return;

    int panel_x = get_ui_panel_x_offset(ui_state->show_ui_panel);
    int panel_w = UI_PANEL_WIDTH;
    int panel_h = SCREEN_HEIGHT;

    // Panel background
    set_color(28, 30, 34, 255); // dark charcoal
    draw_filled_rectangle(panel_x, 0, panel_w, panel_h);

    // Header bar
    int header_h = 72;
    set_color(42, 46, 54, 255); // slightly lighter stripe
    draw_filled_rectangle(panel_x, 0, panel_w, header_h);

    // Title
    draw_text_centered("Chess Game", panel_x + 14, 8, panel_w - 28, 255, 255, 255, large_font);

    // Small turn indicator circle + text
    int badge_x = panel_x + 18;
    int badge_y = 45;
    int badge_r = 8;
    if (ui_state->current_player == WHITE) {
        set_color(240, 240, 240, 255); // white indicator
    } else {
        set_color(40, 40, 40, 255); // dark indicator for black
    }

    draw_filled_rectangle(badge_x - badge_r, badge_y - badge_r, badge_r*2, badge_r*2);

    char turn_text[32];
    sprintf(turn_text, "Turn: %s", ui_state->current_player == WHITE ? "White" : "Black");
    draw_text(turn_text, panel_x + 35, 38, 220, 220, 220, main_font);

    // Game status (Check / Checkmate / Stalemate) - only show if not playing
    if (ui_state->is_in_check || ui_state->is_checkmate || ui_state->is_stalemate) {
        const char *status_text = "Check!";
        if (ui_state->is_checkmate) status_text = "Checkmate";
        if (ui_state->is_stalemate) status_text = "Stalemate";
        draw_text_centered(status_text, panel_x + 14, 58, panel_w - 28, 255, 200, 100, main_font);
    }

    // Controls / toggles block
    int ctrl_y = header_h + 16;
    int xpad = 16;
    draw_text("Controls", panel_x + xpad, ctrl_y, 200, 200, 200, main_font);
    ctrl_y += 22;
    draw_text("TAB - Toggle Panel    R - Reset Board", panel_x + xpad, ctrl_y, 160, 160, 160, main_font);
    ctrl_y += 20;
    draw_text("C - Toggle Coords     H - Toggle Hints", panel_x + xpad, ctrl_y, 160, 160, 160, main_font);
    ctrl_y += 24;

    // Settings block (small badges)
    int badge_w = panel_w - xpad * 2;
    int badge_h = 40;
    set_color(34, 36, 40, 220); // subtle dark strip
    draw_filled_rectangle(panel_x + xpad, ctrl_y, badge_w, badge_h);
    char coords_text[64];
    sprintf(coords_text, "Coordinates: %s    Move Hints: %s",
            ui_state->show_coordinates ? "ON" : "OFF",
            ui_state->show_move_hints ? "ON" : "OFF");
    draw_text(coords_text, panel_x + xpad + 8, ctrl_y + 12, 190, 190, 190, main_font);
    ctrl_y += badge_h + 20;

    // Move History header
    draw_text("Move History", panel_x + xpad, ctrl_y, 200, 200, 200, main_font);
    ctrl_y += 22;

    // Move history container
    int mh_x = panel_x + xpad;
    int mh_w = panel_w - xpad * 2;
    int mh_h = panel_h - ctrl_y - 24;
    // container background
    set_color(24, 26, 30, 200);
    draw_filled_rectangle(mh_x, ctrl_y, mh_w, mh_h);
    // container border
    set_color(60, 60, 60, 150);
    draw_rectangle(mh_x, ctrl_y, mh_w, mh_h);

    // Insets inside move history
    int inner_x = mh_x + 10;
    int inner_y = ctrl_y + 10;
    int inner_w = mh_w - 20;
    int inner_h = mh_h - 20;

    // Determine how many rows fit (each row height)
    int row_h = 32;
    int rows_can_show = inner_h / row_h;
    if (rows_can_show < 1) return;

    // Compute which rows to show: show last rows (scroll-to-end behavior)
    int total_rows = (ui_state->move_count + 1) / 2; // each row = a full move (white+black)
    int start_row = total_rows - rows_can_show;
    if (start_row < 0) start_row = 0;

    // Column geometry: Move# | White | Black
    int col_num_w = 34;
    int col_white_w = (inner_w - col_num_w) / 2;
    int col_black_w = inner_w - col_num_w - col_white_w;

    // Draw rows
    for (int r = 0; r < rows_can_show; ++r) {
        int row_idx = start_row + r;
        int row_y = inner_y + r * row_h;

        // alternating row background
        if ((row_idx % 2) == 0) {
            set_color(34, 36, 40, 160); // slightly lighter
            draw_filled_rectangle(inner_x, row_y, inner_w, row_h);
        }

        // Move number
        char move_num_buf[8];
        sprintf(move_num_buf, "%d.", row_idx + 1);
        set_color(150, 150, 150, 255);
        draw_text_centered(move_num_buf, inner_x, row_y + 8, col_num_w, 180, 180, 180, main_font);

        // white and black move indices in move_history
        int white_index = row_idx * 2;
        int black_index = white_index + 1;

        // last move highlight: if the last move index matches either white_index or black_index, highlight row
        int last_move_index = ui_state->move_count - 1;
        int highlight = (last_move_index == white_index) || (last_move_index == black_index);

        if (highlight) {
            // subtle highlight behind the move text
            set_color(60, 90, 150, 80);
            draw_filled_rectangle(inner_x + col_num_w, row_y + 4, inner_w - col_num_w, row_h - 8);
        }

        // White move text (left column)
        if (white_index < ui_state->move_count && ui_state->move_history[white_index]) {
            set_color(230, 230, 230, 255);
            draw_text(ui_state->move_history[white_index], inner_x + col_num_w + 8, row_y + 8, 220, 220, 220, main_font);
        }

        // Black move text (right column)
        if (black_index < ui_state->move_count && ui_state->move_history[black_index]) {
            set_color(200, 200, 200, 255);
            draw_text(ui_state->move_history[black_index], inner_x + col_num_w + col_white_w + 8, row_y + 8, 190, 190, 190, main_font);
        }
    }
}

void draw_board_coordinates_dynamic(GameUIState *ui_state) {
    if (!ui_state->show_coordinates) return;
    
    int panel_visible = ui_state->show_ui_panel;
    int square_size = get_square_size(panel_visible);
    int board_x = get_board_x_offset(panel_visible);
    int board_y = get_board_y_offset(panel_visible);
    int board_size = get_board_size(panel_visible);
    
    // Draw file letters (a-h) at bottom
    for (int file = 0; file < 8; file++) {
        char file_letter[2] = {'a' + file, '\0'};
        int x = board_x + file * square_size + square_size / 2 - 5; // Center text
        int y = board_y + board_size + 8; // Position below board
        draw_text(file_letter, x, y, 150, 150, 150, main_font);
    }
    
    // Draw rank numbers (1-8) on left side
    for (int rank = 0; rank < 8; rank++) {
        char rank_number[2] = {'8' - rank, '\0'};
        int x = board_x - 20; // Position to the left of board
        int y = board_y + rank * square_size + square_size / 2 - 8; // Center vertically
        draw_text(rank_number, x, y, 150, 150, 150, main_font);
    }
}

void highlight_last_move(GameUIState *ui_state) {
    if (ui_state->last_move_from >= 0 && ui_state->last_move_to >= 0) {
        int square_size = get_square_size(ui_state->show_ui_panel);
        
        // Highlight from square
        int from_x = ui_state->last_move_from % 8;
        int from_y = ui_state->last_move_from / 8;
        chess_coordinates_to_screen_dynamic(&from_x, &from_y, ui_state);
        
        // Convert chess coordinates to board drawing coordinates
        int chess_x = ui_state->last_move_from % 8;
        int chess_y = ui_state->last_move_from / 8;
        int board_row = 7 - chess_y;  // Flip Y to match board drawing
        int board_col = chess_x;
        
        if ((board_row + board_col) % 2 == 0) {
            // Light square - use darker yellow for contrast
            set_color(180, 150, 0, 100);
        } else {
            // Dark square - use brighter yellow for contrast
            set_color(255, 220, 50, 100);
        }
        draw_filled_rectangle(from_x, from_y, square_size, square_size);
        
        // Highlight to square
        int to_x = ui_state->last_move_to % 8;
        int to_y = ui_state->last_move_to / 8;
        chess_coordinates_to_screen_dynamic(&to_x, &to_y, ui_state);
        
        // Convert chess coordinates to board drawing coordinates
        chess_x = ui_state->last_move_to % 8;
        chess_y = ui_state->last_move_to / 8;
        board_row = 7 - chess_y;  // Flip Y to match board drawing
        board_col = chess_x;
        
        if ((board_row + board_col) % 2 == 0) {
            set_color(180, 150, 0, 100);
        } else {
            set_color(255, 220, 50, 100);
        }
        draw_filled_rectangle(to_x, to_y, square_size, square_size);
    }
}

void highlight_selected_square(GameUIState *ui_state) {
    if (ui_state->selected_square >= 0) {
        int square_size = get_square_size(ui_state->show_ui_panel);
        int x = ui_state->selected_square % 8;
        int y = ui_state->selected_square / 8;
        chess_coordinates_to_screen_dynamic(&x, &y, ui_state);
        
        // Convert chess coordinates to board drawing coordinates
        int chess_x = ui_state->selected_square % 8;
        int chess_y = ui_state->selected_square / 8;
        int board_row = 7 - chess_y;  // Flip Y to match board drawing
        int board_col = chess_x;
        
        if ((board_row + board_col) % 2 == 0) {
            // Light square - use darker selection color
            set_color(LIGHT_SQUARE_SELECTION[0], LIGHT_SQUARE_SELECTION[1], LIGHT_SQUARE_SELECTION[2], LIGHT_SQUARE_SELECTION[3]);
        } else {
            // Dark square - use even darker selection color
            set_color(DARK_SQUARE_SELECTION[0], DARK_SQUARE_SELECTION[1], DARK_SQUARE_SELECTION[2], DARK_SQUARE_SELECTION[3]);
        }
        
        draw_filled_rectangle(x, y, square_size, square_size);
    }
}