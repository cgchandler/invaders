#ifndef STARFIELD_H
#define STARFIELD_H

#include <c64/types.h>

// Initialize with a specific number of stars (max 50)
void starfield_init(unsigned char char_base_index, byte* target_screen, unsigned char num_stars);

// Now accepts screen pointer for collision peeking
void starfield_update_motion(byte* screen);

void starfield_render(byte* target_screen);
void starfield_set_speed(unsigned char frames_per_step);

#endif