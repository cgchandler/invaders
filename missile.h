// © 2026 Christopher G Chandler
// Licensed under the MIT License. See LICENSE file in the project root.

#ifndef MISSILE_H
#define MISSILE_H

/*
 * missile.h
 * Module: Player missile
 * Purpose: Manage player missile lifecycle and collisions.
 */

#include "config.h"

void missile_init(void);
void missile_update(void);
void missile_render(void);

// Encapsulated missile state (module-level)
typedef struct {
	unsigned char active;
	unsigned int x;
	unsigned int y;
} missile_state;

// Accessor for missile state
missile_state* missile_get_state(void);

#endif