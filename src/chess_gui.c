#include "chess_gui.h"
#include "gui.h"
#include <stdlib.h>
#include <string.h>

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
    // Give a bit more vertical space so small header widgets don't overlap
    ctrl_y += 28;

    // Move history container
    int mh_x = panel_x + xpad;
    int mh_w = panel_w - xpad * 2;
    int mh_h = panel_h - ctrl_y - 24;
    // Draw follow-mode indicator in header area (top-right)
    int box_size = 14;
    int follow_x = mh_x + mh_w - xpad - box_size;
    int follow_y = ctrl_y - 26; // sit slightly above the container
    // subtle background for checkbox
    set_color(40, 42, 46, 180);
    draw_filled_rectangle(follow_x, follow_y, box_size, box_size);
    // lighter border
    set_color(110, 110, 110, 180);
    draw_rectangle(follow_x, follow_y, box_size, box_size);
    // Draw a subtle marker to indicate default auto-follow behavior (no toggle)
    set_color(110, 110, 110, 120);
    draw_rectangle(follow_x, follow_y, box_size, box_size);
    draw_text("Auto", follow_x - 46, follow_y, 120, 120, 120, main_font);
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

    // Compute which rows to show with scroll support
    int total_rows = (ui_state->actual_move_count + 1) / 2; // each row = a full move (white+black)

    // Maximum scroll offset that still leaves rows_can_show visible
    int max_offset = total_rows - rows_can_show;
    if (max_offset < 0) max_offset = 0;

    int start_row = ui_state->move_history_scroll_offset;

    // Auto-scroll to latest only when viewing current position AND the user
    // hasn't manually scrolled away from the bottom. This allows manual
    // scrolling while still keeping the automatic "follow" behavior when
    // the view was already at the end.
    if (ui_state->viewing_move_index == -1) {
        if (ui_state->move_history_scroll_offset >= max_offset) {
            start_row = max_offset;
            ui_state->move_history_scroll_offset = start_row;
        } else {
            // Keep user's manually adjusted offset; don't force to end.
            start_row = ui_state->move_history_scroll_offset;
        }
    }

    // Clamp scroll offset (safety)
    if (start_row < 0) start_row = 0;
    if (start_row > max_offset) start_row = max_offset;

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

        // White move text (left column)
        if (white_index < ui_state->actual_move_count && ui_state->move_history[white_index]) {
            // Check if this white move should be highlighted
            int white_viewing_highlight = (ui_state->viewing_move_index == white_index);
            int white_last_move_highlight = (ui_state->actual_move_count - 1 == white_index);
            
            // Highlight individual white move if needed
            if (white_viewing_highlight && ui_state->viewing_move_index != -1) {
                // Yellow highlight for viewed position (when not current)
                set_color(150, 120, 30, 100);
                draw_filled_rectangle(inner_x + col_num_w, row_y + 4, col_white_w, row_h - 8);
            } else if (white_last_move_highlight && ui_state->viewing_move_index == -1) {
                // Blue highlight for current position (when viewing current)
                set_color(60, 90, 150, 80);
                draw_filled_rectangle(inner_x + col_num_w, row_y + 4, col_white_w, row_h - 8);
            }
            
            set_color(230, 230, 230, 255);
            draw_text(ui_state->move_history[white_index], inner_x + col_num_w + 8, row_y + 8, 220, 220, 220, main_font);
        }

        // Black move text (right column)
        if (black_index < ui_state->actual_move_count && ui_state->move_history[black_index]) {
            // Check if this black move should be highlighted
            int black_viewing_highlight = (ui_state->viewing_move_index == black_index);
            int black_last_move_highlight = (ui_state->actual_move_count - 1 == black_index);
            
            // Highlight individual black move if needed
            if (black_viewing_highlight && ui_state->viewing_move_index != -1) {
                // Yellow highlight for viewed position (when not current)
                set_color(150, 120, 30, 100);
                draw_filled_rectangle(inner_x + col_num_w + col_white_w, row_y + 4, col_black_w, row_h - 8);
            } else if (black_last_move_highlight && ui_state->viewing_move_index == -1) {
                // Blue highlight for current position (when viewing current)
                set_color(60, 90, 150, 80);
                draw_filled_rectangle(inner_x + col_num_w + col_white_w, row_y + 4, col_black_w, row_h - 8);
            }
            
            set_color(200, 200, 200, 255);
            draw_text(ui_state->move_history[black_index], inner_x + col_num_w + col_white_w + 8, row_y + 8, 190, 190, 190, main_font);
        }
    }

    // Draw scrollbar on right side of move history
    if (total_rows > rows_can_show) {
        int scrollbar_w = 8;
        int scrollbar_x = mh_x + mh_w - 6; // small inset
        int scrollbar_y = inner_y;
        int scrollbar_h = inner_h;

        // Background track
        set_color(30, 32, 36, 200);
        draw_filled_rectangle(scrollbar_x, scrollbar_y, scrollbar_w, scrollbar_h);

        // Thumb size proportional to visible / total
        float thumb_hf = (float)rows_can_show / (float)total_rows;
        if (thumb_hf < 0.05f) thumb_hf = 0.05f; // min size
        int thumb_h = (int)(thumb_hf * scrollbar_h);

        // Thumb position based on offset
        int max_offset = total_rows - rows_can_show;
        if (max_offset < 1) max_offset = 1;
        float thumb_pos_frac = (float)ui_state->move_history_scroll_offset / (float)max_offset;
        int thumb_y = scrollbar_y + (int)(thumb_pos_frac * (scrollbar_h - thumb_h));

        // Draw thumb
        set_color(100, 100, 110, 200);
        draw_filled_rectangle(scrollbar_x + 1, thumb_y + 1, scrollbar_w - 2, thumb_h - 2);
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
    // Only show last move highlights when viewing the current position
    if (ui_state->viewing_move_index != -1) {
        return;  // Don't show last move highlights when viewing historical positions
    }
    
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

// Move history navigation functions
int handle_move_history_click(int mouse_x, int mouse_y, GameUIState *ui_state) {
    if (!ui_state->show_ui_panel) return 0;
    
    // Calculate move history panel bounds using the same logic as draw_ui_panel
    int panel_x = get_ui_panel_x_offset(ui_state->show_ui_panel);
    int panel_w = UI_PANEL_WIDTH;
    int panel_h = SCREEN_HEIGHT;
    int header_h = 72;
    int xpad = 16;
    
    // Calculate ctrl_y the same way as in draw_ui_panel
    int ctrl_y = header_h + 16;  // Controls start
    ctrl_y += 22;  // "Controls" text
    ctrl_y += 20;  // First line of controls
    ctrl_y += 24;  // Second line of controls + spacing
    ctrl_y += 40 + 20;  // Settings badge height + spacing
    ctrl_y += 28;  // "Move History" text + extra spacing (match draw_ui_panel)
    
    // Move history container bounds (same as draw_ui_panel)
    int mh_x = panel_x + xpad;
    int mh_w = panel_w - xpad * 2;
    int mh_h = panel_h - ctrl_y - 24;
    
    // Check if click is within move history panel
    if (mouse_x < mh_x || mouse_x > mh_x + mh_w || 
        mouse_y < ctrl_y || mouse_y > ctrl_y + mh_h) {
        return 0;
    }

    // Detect click on follow checkbox (top-right of panel header)
    int box_size = 14;
    int follow_x = mh_x + mh_w - xpad - box_size;
    int follow_y = ctrl_y - 26;
    // clicking the small marker doesn't toggle behavior — ignore clicks here
    
    // If no moves yet, return early
    if (ui_state->actual_move_count == 0) {
        return 0;
    }
    
    // Calculate which move was clicked
    int inner_x = mh_x + 10;
    int inner_y = ctrl_y + 10;
    int inner_w = mh_w - 20;
    int inner_h = mh_h - 20;
    
    int row_h = 32;
    int rows_can_show = inner_h / row_h;
    if (rows_can_show < 1) return 0;
    
    // Use the current scroll offset instead of auto-calculating
    int start_row = ui_state->move_history_scroll_offset;
    
    int clicked_row = (mouse_y - inner_y) / row_h;
    if (clicked_row < 0 || clicked_row >= rows_can_show) return 0;
    
    int row_idx = start_row + clicked_row;
    
    // Determine which column (white or black move) was clicked
    int col_num_w = 34;
    int col_white_w = (inner_w - col_num_w) / 2;
    int relative_x = mouse_x - inner_x;
    
    int move_index = -1;
    if (relative_x >= col_num_w && relative_x < col_num_w + col_white_w) {
        // White move clicked
        move_index = row_idx * 2;
    } else if (relative_x >= col_num_w + col_white_w) {
        // Black move clicked
        move_index = row_idx * 2 + 1;
    }
    
    // Only navigate if the move exists
    if (move_index >= 0 && move_index < ui_state->actual_move_count) {
    navigate_to_move(ui_state, move_index);
    // If the user clicked into history manually, disable follow mode
        // ui_state->follow_mode = 0; // Removed as follow_mode no longer exists
    return 1;
    }
    
    return 0;
}

void navigate_to_move(GameUIState *ui_state, int move_index) {
    // Move index -1 means current position, 0 means after first move, etc.
    ui_state->viewing_move_index = move_index;
    
    // Update UI state based on the viewing position
    if (move_index == -1 || move_index == ui_state->actual_move_count - 1) {
        // Viewing current position
        ui_state->move_count = ui_state->actual_move_count;
        ui_state->viewing_move_index = -1;
    } else {
        // Viewing historical position
        ui_state->move_count = move_index + 1;
    }
    
    // Clear selection when navigating
    ui_state->selected_square = -1;
}

void save_board_position(GameUIState *ui_state, ChessBoard *board) {
    // Expand arrays if needed
    if (ui_state->actual_move_count >= ui_state->move_history_capacity) {
        expand_move_history(ui_state);
    }
    
    // Save the current board position to history
    int index = ui_state->actual_move_count;
    if (index < ui_state->move_history_capacity) {
        ui_state->board_history[index] = *board;
    }
}

ChessBoard* get_viewing_board(GameUIState *ui_state, ChessBoard *current_board) {
    // Return the board position we should be viewing
    if (ui_state->viewing_move_index == -1) {
        // Viewing current position
        return current_board;
    } else {
        // Viewing historical position
        int history_index = ui_state->viewing_move_index + 1;
        if (history_index >= 0 && history_index < ui_state->move_history_capacity) {
            return &ui_state->board_history[history_index];
        }
    }
    return current_board;
}

// Helper function to calculate visible rows consistently
int calculate_visible_rows(void) {
    // Use the same calculation as in draw_ui_panel
    int panel_h = SCREEN_HEIGHT;
    int header_h = 72;
    int xpad = 16;
    
    // Calculate move history panel height (match draw_ui_panel spacing)
    int ctrl_y = header_h + 16;
    ctrl_y += 22;  // "Controls"
    ctrl_y += 20;  // first control line
    ctrl_y += 24;  // second control line
    ctrl_y += 40 + 20; // badge
    ctrl_y += 28; // move history header spacing
    int mh_h = panel_h - ctrl_y - 24;
    int inner_h = mh_h - 20;
    int row_h = 32;
    int rows_can_show = inner_h / row_h;
    
    return (rows_can_show < 1) ? 1 : rows_can_show;
}

// Dynamic move history management functions
void init_move_history(GameUIState *ui_state) {
    ui_state->move_history_capacity = 100;  // Initial capacity
    ui_state->move_history = malloc(ui_state->move_history_capacity * sizeof(char*));
    ui_state->board_history = malloc(ui_state->move_history_capacity * sizeof(ChessBoard));
    ui_state->actual_move_count = 0;
    ui_state->viewing_move_index = -1;
    ui_state->move_history_scroll_offset = 0;
    
    // Initialize move strings to NULL
    for (int i = 0; i < ui_state->move_history_capacity; i++) {
        ui_state->move_history[i] = NULL;
    }
}

void cleanup_move_history(GameUIState *ui_state) {
    if (ui_state->move_history) {
        for (int i = 0; i < ui_state->actual_move_count; i++) {
            if (ui_state->move_history[i]) {
                free(ui_state->move_history[i]);
            }
        }
        free(ui_state->move_history);
        ui_state->move_history = NULL;
    }
    
    if (ui_state->board_history) {
        free(ui_state->board_history);
        ui_state->board_history = NULL;
    }
    
    ui_state->move_history_capacity = 0;
    ui_state->actual_move_count = 0;
}

void expand_move_history(GameUIState *ui_state) {
    int new_capacity = ui_state->move_history_capacity * 2;
    
    // Expand move history array
    char **new_move_history = realloc(ui_state->move_history, new_capacity * sizeof(char*));
    if (!new_move_history) return; // Failed to allocate
    ui_state->move_history = new_move_history;
    
    // Expand board history array
    ChessBoard *new_board_history = realloc(ui_state->board_history, new_capacity * sizeof(ChessBoard));
    if (!new_board_history) return; // Failed to allocate
    ui_state->board_history = new_board_history;
    
    // Initialize new slots to NULL
    for (int i = ui_state->move_history_capacity; i < new_capacity; i++) {
        ui_state->move_history[i] = NULL;
    }
    
    ui_state->move_history_capacity = new_capacity;
}

void handle_move_history_keyboard(GameUIState *ui_state, int key) {
    if (!ui_state->show_ui_panel || ui_state->actual_move_count == 0) return;
    
    int old_viewing_index = ui_state->viewing_move_index;
    
    switch (key) {
        case SDLK_UP:
            // Navigate to previous move
            if (ui_state->viewing_move_index == -1) {
                // Currently viewing latest, go to previous move
                if (ui_state->actual_move_count > 0) {
                    navigate_to_move(ui_state, ui_state->actual_move_count - 2);
                }
            } else if (ui_state->viewing_move_index > 0) {
                navigate_to_move(ui_state, ui_state->viewing_move_index - 1);
            }
            break;
            
        case SDLK_DOWN:
            // Navigate to next move
            if (ui_state->viewing_move_index != -1) {
                if (ui_state->viewing_move_index < ui_state->actual_move_count - 1) {
                    navigate_to_move(ui_state, ui_state->viewing_move_index + 1);
                } else {
                    // Go to current position
                    navigate_to_move(ui_state, -1);
                }
            }
            break;
            
        case SDLK_HOME:
            // Go to start of game
            if (ui_state->actual_move_count > 0) {
                navigate_to_move(ui_state, 0);
            }
            break;
            
        case SDLK_END:
            // Go to current position
            navigate_to_move(ui_state, -1);
            break;
            
        case SDLK_PAGEUP:
            scroll_move_history(ui_state, -1);
            break;
            
        case SDLK_PAGEDOWN:
            scroll_move_history(ui_state, 1);
            break;
    }
    
    // Auto-scroll to ensure the selected move is visible (if move changed)
    if (ui_state->viewing_move_index != old_viewing_index) {
        ensure_move_visible(ui_state);
    }
}

void scroll_move_history(GameUIState *ui_state, int direction) {
    int total_rows = (ui_state->actual_move_count + 1) / 2;
    int max_visible_rows = calculate_visible_rows();
    // Manual scroll does not toggle any UI flag (keep user's offset unchanged)
    
    if (direction < 0) {
        // Scroll up
        ui_state->move_history_scroll_offset = 
            (ui_state->move_history_scroll_offset > 0) ? 
            ui_state->move_history_scroll_offset - 1 : 0;
    } else {
        // Scroll down
        int max_offset = total_rows - max_visible_rows;
        if (max_offset < 0) max_offset = 0;
        ui_state->move_history_scroll_offset = 
            (ui_state->move_history_scroll_offset < max_offset) ? 
            ui_state->move_history_scroll_offset + 1 : max_offset;
    }
}

void ensure_move_visible(GameUIState *ui_state) {
    int max_visible_rows = calculate_visible_rows();
    
    if (ui_state->viewing_move_index == -1) {
        // Viewing current position - auto-scroll to end
        int total_rows = (ui_state->actual_move_count + 1) / 2;
        int max_offset = total_rows - max_visible_rows;
        if (max_offset < 0) max_offset = 0;
        ui_state->move_history_scroll_offset = max_offset;
        return;
    }
    
    // Calculate which row the selected move is in
    int move_row = ui_state->viewing_move_index / 2;
    
    // Calculate visible range
    int first_visible_row = ui_state->move_history_scroll_offset;
    int last_visible_row = first_visible_row + max_visible_rows - 1;
    
    // Add some margin to make scrolling feel better (scroll a bit early)
    int scroll_margin = 1;
    
    // Adjust scroll offset if needed
    if (move_row < first_visible_row + scroll_margin) {
        // Move is too close to top - scroll up
        ui_state->move_history_scroll_offset = move_row - scroll_margin;
        if (ui_state->move_history_scroll_offset < 0) {
            ui_state->move_history_scroll_offset = 0;
        }
    } else if (move_row > last_visible_row - scroll_margin) {
        // Move is too close to bottom - scroll down
        ui_state->move_history_scroll_offset = move_row - max_visible_rows + 1 + scroll_margin;
        
        // Ensure we don't scroll past the end
        int total_rows = (ui_state->actual_move_count + 1) / 2;
        int max_offset = total_rows - max_visible_rows;
        if (max_offset < 0) max_offset = 0;
        
        if (ui_state->move_history_scroll_offset > max_offset) {
            ui_state->move_history_scroll_offset = max_offset;
        }
        if (ui_state->move_history_scroll_offset < 0) {
            ui_state->move_history_scroll_offset = 0;
        }
    }
}