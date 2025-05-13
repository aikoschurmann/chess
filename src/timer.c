#include "timer.h"

Uint64 start_counter = 0;
Uint64 end_counter = 0;
Uint64 frequency = 0; // Frequency of the performance counter


void start_timer() {
    start_counter = SDL_GetPerformanceCounter(); // Record the current performance counter
}

void stop_timer(const char* section_name) {
    end_counter = SDL_GetPerformanceCounter(); // Record the current performance counter
    Uint64 elapsed = end_counter - start_counter; // Elapsed counter ticks
    double elapsed_ms = (double)elapsed / (double)frequency * 1000.0; // Convert to milliseconds
    printf("Time taken for %s: %.2f ms\n", section_name, elapsed_ms);
}

void initialize_timer() {
    frequency = SDL_GetPerformanceFrequency(); // Get the frequency of the performance counter (ticks per second)
}
