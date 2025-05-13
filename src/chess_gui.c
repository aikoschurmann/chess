#include "chess_gui.h"
#include "gui.h"


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



void draw_chess_board() {
    const int square_size = (SCREEN_WIDTH - 2 * PADDING) / 8;

    const int light_square_color[4] = {255, 206, 158, 255};
    const int dark_square_color[4] = {209, 139, 71, 255};

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            // Alternate color based on sum of row and column indices
            if ((i + j) % 2 == 0) {
                set_color(light_square_color[0], light_square_color[1], light_square_color[2], light_square_color[3]);
            } else {
                set_color(dark_square_color[0], dark_square_color[1], dark_square_color[2], dark_square_color[3]);
            }

            draw_rectangle(PADDING + j * square_size, i * square_size, square_size, square_size);
        }
    }
}


void chess_coordinates_to_screen(int *x, int *y) {
    
    *x = PADDING + *x * (SCREEN_WIDTH - 2 * PADDING) / 8;
    
    *y = SCREEN_HEIGHT - (*y + 1) * (SCREEN_WIDTH - 2 * PADDING) / 8;
}

void screen_to_chess_coordinates(int *x, int *y) {

    int TILE_SIZE = (SCREEN_WIDTH - 2 * PADDING) / 8;

    *x = (*x - PADDING) / TILE_SIZE;
    *y = 7 - *y / TILE_SIZE;
}

void draw_bitboard_mask(Bitboard bitboard, int r, int g, int b, int a) {
    for (int i = 0; i < 64; i++) {
        if (bitboard & (1ULL << i)) {
            int x = i % 8;
            int y = i / 8;
            chess_coordinates_to_screen(&x, &y);
            set_color(r, g, b, a);
            draw_rectangle(x, y, (SCREEN_WIDTH - 2 * PADDING) / 8, (SCREEN_WIDTH - 2 * PADDING) / 8);
        }
    }
}

void draw_pieces_for_color(Bitboard pieces[6], SDL_Texture *sprites[6]) {
    for (int i = 0; i < 6; i++) {
        Bitboard current_pieces = pieces[i];

        while (current_pieces) {
            int index = __builtin_ctzll(current_pieces);  // Find the lowest set bit
            int x = index % 8;
            int y = index / 8;

            chess_coordinates_to_screen(&x, &y);  // Convert to screen coordinates
            SDL_Texture *sprite = sprites[i];  // Get the appropriate sprite
            draw_sprite(sprite, x, y);  // Draw the piece

            current_pieces &= current_pieces - 1;  // Clear the lowest set bit
        }
    }
}

void draw_pieces(ChessBoard *board) {
    draw_pieces_for_color(board->pieces[WHITE], white_sprites);
    draw_pieces_for_color(board->pieces[BLACK], black_sprites);
}

      