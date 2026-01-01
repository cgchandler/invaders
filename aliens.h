#ifndef ALIENS_H
#define ALIENS_H

#include <c64/types.h>

// Initialize the swarm
// screen: Pointer to the screen memory (0x4400)
void aliens_init(byte* screen);

// Calculate logic, collisions, and movement. 
// Call this anywhere in the main loop (does not need VBlank)
void aliens_update(void);

// Draw the aliens to the screen. 
// MUST be called directly after vic_waitFrame() (inside VBlank) to prevent tearing.
void aliens_render(byte* screen);

// Optional: check for collision with a player bullet (x,y)
// Returns 1 if hit, 0 if miss. Handles destroying the alien.
int aliens_check_hit(unsigned char col, unsigned char row);

int aliens_cleared(void); // Returns 1 if count is 0
void aliens_reset(void);  // Resets positions and brings aliens back to life

extern unsigned char g_alive_count;

#endif