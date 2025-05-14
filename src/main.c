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


//int main(int argc, char const *argv[]) {
//
//    DebugState debugstate = {PAWN, 1, 0};  // Start with Pawn, white's bitboard, draw bitboards off
//    ChessBoard board;
//    ChessMove moves[256];
//    int num_moves = 0;
//
//    ChessMove tile_moves[256];
//    int num_tile_moves = 0;
//    unsigned long long moves_mask = 0;
//
//    int selected_tile = -1;
//
//    // Initialize everything
//    initialize_window("Chess Engine", SCREEN_WIDTH, SCREEN_HEIGHT);
//    initialize_keyboard_state();
//    initialize_sprites();
//    initialize_bitboards();
//    initialize_board(&board);
//    initialize_timer();
//
//    //run_perft_tests_up_to(6); // Run perft tests up to depth 5
//
//
//
//    //board.current_turn = BLACK;
//
//    // Generate moves and bitboard mask
//    //generate_moves(&board, moves, &num_moves);
//    //printf("Number of moves: %d\n", num_moves);
//    //enerate_bitboard_from_moves(moves, num_moves, &moves_mask);
//
//
//    // Main loop
//    while (should_continue) {
//        // Process events (keyboard and mouse)
//        handle_events();
//
//        // Handle key events
//        handle_input(&debugstate); // Handle input and update debugstate
//
//        // Clear screen and render new frame
//        clear_window();
//
//        // Draw the chess board and pieces
//        draw_chess_board();
//        draw_pieces(&board);
//        draw_selected_bitboard(&debugstate, &board); // Draw the selected bitboard (DEBUG)
//        //draw_bitboard_mask(moves_mask, 0, 255, 0, 100); // Draw the moves mask (DEBUG)
//        //draw_bitboard_mask(0x8080808080808080, 255, 0, 255, 100); // Draw the diagonal mask (DEBUG)
//
//        if (is_key_down(SDL_SCANCODE_R)) {
//            // Reset the board
//            initialize_board(&board);
//        }
//
//        if (1) {
//            bot_play(&board);
//            SDL_Delay(250);
//        }
//        
//
//        
//
//        if (mouse_clicked == 1) {
//            int x = mouse_location[0];
//            int y = mouse_location[1];
//
//            screen_to_chess_coordinates(&x, &y);
//            int tile = y * 8 + x;
//            
//            num_moves = 0;
//            num_tile_moves = 0;
//
//            generate_moves(&board, moves, &num_moves);
//            verify_king_safety(&board, moves, &num_moves);
//
//            
//            for (int i = 0; i < num_moves; i++) {
//                if (moves[i].start_tile == tile) {
//                    tile_moves[num_tile_moves++] = moves[i];
//                    selected_tile = tile;
//                }
//            }
//
//            if (num_moves == 0) {
//                selected_tile = -1;
//            }
//
//            if (selected_tile != -1) {
//                for (int i = 0; i < num_moves; i++) {
//                    if (moves[i].start_tile == selected_tile && moves[i].end_tile == tile) {
//                        
//                        apply_move(&board, &moves[i]);
//
//                        break;
//                    }
//                }
//            }
//            
//            generate_bitboard_from_moves(tile_moves, num_tile_moves, &moves_mask);            
//        }
//
//        draw_bitboard_mask(moves_mask, 0, 255, 0, 100);
//        //draw_bitboard_mask(0x0101010101010101, 255, 0, 0, 100);
//        
//        // Present the rendered frame
//        present_window();
//    }
//
//    return 0;
//}
