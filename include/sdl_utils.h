#ifndef SDL_UTILS_H
#define SDL_UTILS_H

#include <SDL.h>
#include <stdio.h>


// Initializes SDL, creates a window and a renderer
int init_SDL(SDL_Window **window, SDL_Renderer **renderer, const char *title, int width, int height);

// Handles the event loop and drawing
void event_loop(SDL_Renderer *renderer, int x_offset, int y_offset, int square_size);

// Cleans up SDL resources
void cleanup_SDL(SDL_Window *window, SDL_Renderer *renderer);

#endif // SDL_UTILS_H
