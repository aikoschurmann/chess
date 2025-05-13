#ifndef CHESS_GUI_H
#define CHESS_GUI_H

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480
#define PADDING 160

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

void draw_chess_board();
void draw_bitboard_mask(Bitboard bitboard, int r, int g, int b, int a);
void draw_pieces(ChessBoard *board);
void chess_coordinates_to_screen(int *x, int *y);
void screen_to_chess_coordinates(int *x, int *y);
void initialize_sprites();

#endif