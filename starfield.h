#ifndef STARFIELD_H
#define STARFIELD_H


/*
 * starfield.h
 * Module: Background starfield
 * Purpose: Create and render background stars; keep CPU usage minimal.
 */

#include "config.h"
#include <c64/types.h>

/* Tunables exposed to callers */
#define MAX_STARS 50

// Initialize with a specific number of stars (max 50)
void starfield_init(unsigned char char_base_index, unsigned char num_stars);

// Now accepts screen pointer for collision peeking
void starfield_update_motion();

void starfield_render();
void starfield_set_speed(unsigned char frames_per_step);

/* Encapsulated starfield state (opaque to callers) */
typedef struct {
	unsigned char char_base;
	unsigned char speed_delay;
	unsigned char speed_counter;
	unsigned char active_stars;
	/* Per-star runtime arrays (size `MAX_STARS`).
	 * These were previously file-local arrays in `starfield.c`.
	 */
	unsigned short curr_pos[MAX_STARS];
	unsigned char  curr_phase[MAX_STARS];
	unsigned char  curr_color[MAX_STARS];

	unsigned short next_pos[MAX_STARS];
	unsigned char  next_phase[MAX_STARS];
	unsigned char  next_color[MAX_STARS];
} starfield_state;

/* Getter to obtain pointer to internal state if needed */
starfield_state* starfield_get_state(void);

#endif