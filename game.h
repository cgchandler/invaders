/* game.h
 * Centralized runtime game state accessor.
 */

#ifndef GAME_H
#define GAME_H

#include <c64/types.h>

/* Game modes used by `game_state` */
typedef enum {
    MODE_INTRO = 0,
    MODE_PLAY = 1,
    MODE_GAME_OVER = 2,
    MODE_LEVEL_DISPLAY = 3
} game_mode_t;

typedef struct {
    unsigned int score;
    unsigned int high_score;
    unsigned int shots_fired;
    unsigned int next_life_score;
    unsigned char level;
    unsigned char max_lives;
    game_mode_t mode;       // Current game mode
} game_state;

extern game_state g_game_state;

/* Accessor implemented in game.c */
game_state* game_get_state(void);

/* NOTE: old `g_*` macro aliases removed — use `game_get_state()` */

#endif /* GAME_H */
