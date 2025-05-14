// bot.h
#ifndef BOT_H
#define BOT_H

#include "chess_board.h"

// Entry point for a bot to select a move
void bot_select_move(ChessBoard *board, ChessMove *out_move);
// Function to play a move for the bot
void bot_play(ChessBoard *board);

#endif