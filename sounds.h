// © 2026 Christopher G Chandler
// Licensed under the MIT License. See LICENSE file in the project root.

#ifndef SOUNDS_H
#define SOUNDS_H

/*
 * sounds.h
 * Module: Sound effects and music triggers
 * Purpose: Initialize and trigger SFX. Keep handlers short and non-blocking.
 */

#include "config.h"

void sound_init(void);
void sound_update(void); // Call this every frame!

// Triggers
void sfx_fire_missile(void);
void sfx_alien_hit(void);
void sfx_player_die(void);
void sfx_game_over(void);
void sfx_high_score(void);
void sfx_march(void); // Call this when aliens move
void sfx_ufo_start(void);
void sfx_ufo_stop(void);
void sfx_bonus_ship_hit(void);

#endif