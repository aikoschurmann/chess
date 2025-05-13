#include "input.h"
#include <SDL.h>

void handle_input(DebugState *debugstate) {
    if (is_key_down(SDL_SCANCODE_LEFT)) {
        debugstate->selected_piece = cycle_bitboard(debugstate->selected_piece, -1); // Go to previous bitboard
    }

    if (is_key_down(SDL_SCANCODE_RIGHT)) {
        debugstate->selected_piece = cycle_bitboard(debugstate->selected_piece, 1); // Go to next bitboard
    }

    if (is_key_down(SDL_SCANCODE_SPACE) || is_key_down(SDL_SCANCODE_UP)) {
        debugstate->draw_white_bitboard_mask = !debugstate->draw_white_bitboard_mask; // Toggle between white and black turns
    }

    if (is_key_down(SDL_SCANCODE_B)) {
        debugstate->should_draw_bitboards = !debugstate->should_draw_bitboards; // Toggle between drawing bitboards and not drawing them
    }
}