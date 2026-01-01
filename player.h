#ifndef PLAYER_H
#define PLAYER_H

// Initialize sprite memory and variables
void player_init(void);

// Read keys (A/D) and update position
// Call this in your "Logic Phase"
void player_update(void);

// Update VIC-II registers (Sprite X, Y, MSB)
// Call this in your "Render Phase" (VBlank)
void player_render(void);

void player_die(void);

void player_reset_position(void);

#endif