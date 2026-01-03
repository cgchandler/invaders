#ifndef PLAYER_H
#define PLAYER_H

/*
 * player.h
 * Module: Player control and sprite handling
 * Purpose: Read input, update player position and update sprite registers.
 */

#include "config.h"

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

/* Encapsulated player state */
typedef struct {
	unsigned char lives;
	unsigned char default_lives;
	unsigned int player_x;
} player_state;

/* Note: legacy `g_*` aliases removed — use `player_get_state()` */

/* Accessor for pointer-based access */
player_state* player_get_state(void);


#endif