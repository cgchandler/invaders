/* game.h
 * Centralized runtime game state accessor.
 */

#ifndef GAME_H
#define GAME_H

#include <c64/types.h>

/* Game modes used by `game_state` */
typedef enum {
    MODE_INTRO = 0,
    MODE_DEMO = 1,
    MODE_PLAY = 2,
    MODE_GAME_OVER = 3,
    MODE_LEVEL_DISPLAY = 4
} game_mode_t;

typedef enum {
    JOYSTICK = 0,
    KEYBOARD = 1
} game_control_t;

/* Top-level mutable game state structure */
typedef struct {
    unsigned int score;
    unsigned int high_score;
    unsigned int shots_fired;
    unsigned int next_life_score;
    unsigned char level;
    unsigned char max_lives;
    game_mode_t mode;       // Current game mode
    game_control_t control; // Input method
    unsigned char demo;     // Non-zero when running in attract/demo mode
} game_state;

extern game_state g_game_state;

/* Accessor implemented in game.c */
game_state* game_get_state(void);

#endif /* GAME_H */
