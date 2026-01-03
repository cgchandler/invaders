#ifndef BONUS_SHIP_H
#define BONUS_SHIP_H

/*
 * bonus_ship.h
 * Module: Bonus ship logic and rendering
 * Purpose: Spawn, move and detect collisions with the bonus ship.
 */

#include "config.h"

// Initializes the bonus ship system
void bonus_init(void);

// Resets the ship state (e.g. on new level or game over)
void bonus_reset(void);

// Handles movement, state changes, and spawning logic
void bonus_update(void);

// Draws the sprite to the screen
void bonus_render(void);

// checks collision with missile (returns points awarded, 0 if miss)
int bonus_check_hit(int m_col, int m_row);

// Encapsulated bonus ship state
typedef struct {
	unsigned char state;
	int x;
	int dir;
	unsigned char timer;
	unsigned char anim_frame;
	unsigned int last_score_val;
	int score_grid_pos;
	unsigned int score_timer;
} bonus_ship_state;

bonus_ship_state* bonus_ship_get_state(void);

#endif