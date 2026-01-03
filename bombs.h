#ifndef BOMBS_H
#define BOMBS_H

/*
 * bombs.h
 * Module: Alien bombs
 * Purpose: Manage falling bombs fired by aliens.
 * Invariants: Call `bombs_update` in logic phase; `bombs_render` in VBlank.
 */

#include "config.h"

void bombs_init(void);
void bombs_update(void);
void bombs_render(void);

// Configuration
#define MAX_BOMBS 5

// Encapsulated bombs state
typedef struct {
	unsigned char active[MAX_BOMBS];
	unsigned int  x[MAX_BOMBS];
	unsigned int  y[MAX_BOMBS];
} bombs_state;

// Accessor for bombs state
bombs_state* bombs_get_state(void);

#endif