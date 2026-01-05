/*
 * config.h
 * Project-wide configuration for Invaders (C64)
 * Centralizes hardware addresses, memory layout constants, and naming conventions.
 *
 * Intended usage:
 * - Include this header from module headers to keep magic numbers centralized.
 * - Expose well-named compile-time constants (e.g. `SCREEN_BASE`).
 * - Document naming conventions and module invariants.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <c64/types.h>

/* --- Naming conventions ---
 * - Exported globals (visible across modules): prefix `g_` (e.g. `g_score`).
 * - File-local static variables/functions: prefix `static_` and mark with
 *   the `static` keyword in the .c file (e.g. `static unsigned char static_led;`).
 * - Functions: use snake_case (e.g. `aliens_update`).
 * - Headers should expose only minimal `extern` handles; prefer accessors
 *   over exporting many globals.
 */

/* --- Common hardware / layout constants (canonical values) ---
 * These values are chosen to match `invaders.c` and the runtime layout.
 */

/* Screen buffer base (text RAM at $4400 in VIC bank 1) */
//#define SCREEN_BASE 0x4400

/* Sprite and charset addresses (SPRITES_ADDR is the byte address used by memcpy)
 * Sprite pointer indexes (PLAYER_SPRITE_PTR, MISSILE_SPRITE_PTR) are the
 * values written into the screen pointer table (0..255). invaders.c uses 32
 * and 33 respectively.
 */
//#define SPRITES_ADDR 0x4800
#define PLAYER_SPRITE_PTR 32
#define MISSILE_SPRITE_PTR 33

/* Example VIC/CIA addresses (uncomment if you need them elsewhere)
 * #define VIC_BASE 0xD000
 * #define CIA1_BASE 0xDC00
 */

/* --- Shared runtime state (globals) ---
 * Use `game.h` for top-level mutable game state (score, level, shots).
 */
#include "game.h"

/* Common screen/font pointers (defined in `invaders.c`) */
extern byte* const Screen;
extern byte* const Font;
extern byte* const Color;
extern byte* const Sprites;

/* Display helpers implemented in `invaders.c` */
void update_score_display(void);
void update_lives_display(void);
void update_level(void);

/* Game control */
void game_over(void);

/* End of canonical config.h - see top for definitions */

#endif /* CONFIG_H */