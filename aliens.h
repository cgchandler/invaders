// © 2026 Christopher G Chandler
// Licensed under the MIT License. See LICENSE file in the project root.

#ifndef ALIENS_H
#define ALIENS_H

/*
 * aliens.h
 * Module: Alien swarm logic and rendering
 * Purpose: Manage alien positions, collisions, and rendering timing.
 * Invariants: `aliens_render` must be called from VBlank (after vic_waitFrame()).
 */

#include "config.h"
#include <c64/types.h>

// Initialize the swarm
// screen: Pointer to the screen memory (0x4400)
void aliens_init();

// Calculate logic, collisions, and movement. 
// Call this anywhere in the main loop (does not need VBlank)
void aliens_update(void);

// Draw the aliens to the screen. 
// MUST be called directly after vic_waitFrame() (inside VBlank) to prevent tearing.
void aliens_render();

// Optional: check for collision with a player bullet (x,y)
// Returns 1 if hit, 0 if miss. Handles destroying the alien.
int aliens_check_hit(unsigned char col, unsigned char row);

int aliens_cleared(void); // Returns 1 if count is 0
void aliens_reset(void);  // Resets positions and brings aliens back to life

/* Encapsulated aliens state */
typedef struct {
	unsigned char alive_count;
	int grid_x;
	int grid_y;
	/* Module-local runtime fields migrated from aliens.c */
	int old_grid_x;
	int old_grid_y;
	int dir;
	int next_dir;
	int state;
	unsigned char anim_frame;
	unsigned char timer;
	unsigned char current_delay;
	unsigned char render_dirty;
} aliens_state;

/* Accessor for the state (pointer for pass-by-ref) */
aliens_state* aliens_get_state(void);

// Returns 1 if a shooter was found, populating x/y with pixel coordinates
int aliens_get_random_shooter(int* out_x, int* out_y);

void aliens_reset_postion(aliens_state* a); // Resets only position to starting point

// Debug
void aliens_debug_speed(void);

#endif