#ifndef TIMER_H
#define TIMER_H

#include <SDL.h>

extern Uint64 start_counter;
extern Uint64 end_counter;
extern Uint64 frequency;

void start_timer();
void stop_timer(const char* section_name);

void initialize_timer();

#endif